#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>

#include "memcache.hh"
#include "gen_memcache.hh"
#include "socket_buf.hh"
#include "util.hh"

using namespace std;
using namespace std::placeholders;

// XXX: Future work:
// - Creating a pool of key requests per memcache generator but only need one shared
// - Actually parse return value
// - Support mixed GET/SET workloads
// - Support choosing key with a distribution
// - Support variable value size with SET workload

/**
 * Create a memcache request for a given key id.
 */
static void create_get_req(char *buf, uint64_t id)
{
    MemcHeader header;

    header.type = MEMC_REQUEST;
    header.cmd = MEMC_CMD_GET;
    header.keylen = htons(memcache::KEYLEN);
    header.extralen = 0;
    header.datatype = 0;
    header.status = htons(MEMC_OK);
    header.bodylen = htonl(memcache::KEYLEN);
    header.opaque = 0;
    header.version = 0;

    static_assert(memcache::KEYLEN >= 30, "keys are 30 chars long, not enough space");

    memcpy(buf, &header, MEMC_HEADER_SIZE);
    snprintf(buf + MEMC_HEADER_SIZE, memcache::KEYLEN, "key-%026" PRIu64, id);
}

/**
 * Construct.
 */
memcache::memcache(const Config &cfg, std::mt19937 &rand) noexcept
  : cfg_{cfg},
    rand_{rand},
    cb_{bind(&memcache::recv_response, this, _1, _2, _3, _4, _5, _6, _7)},
    requests_{},
    seqid_{rand_()} // start from random sequence id
{
    UNUSED(cfg_);

    for (size_t i = 1; i <= KEYS; i++) {
        // create all needed requests upfront
        create_get_req(keys_[i - 1], i);
    }
}

/**
 * Generate and send a new request.
 */
void memcache::_send_request(bool measure, request_cb cb)
{
    // create our request
    memreq &req = requests_.queue_emplace(measure, cb);
    char *key = keys_[seqid_++ % KEYS];

    // add req to write queue
    size_t n = KEYREQ, n1 = n;
    auto wptrs = sock_.write_prepare(n1);
    memcpy(wptrs.first, key, n1);
    if (n != n1) {
        memcpy(wptrs.second, key + n1, n - n1);
    }
    sock_.write_commit(n);

    // add response to read queue
    ioop io(24, &req, cb_);
    sock_.read(io);
}

/**
 * Handle parsing a response from a previous request.
 */
void memcache::recv_response(Sock *s, void *data, char *seg1, size_t n,
                             char *seg2, size_t m, int status)
{
    UNUSED(seg1);
    UNUSED(seg2);

    if (&sock_ != s) { // ensure right callback
        throw runtime_error(
          "memcache::recv_response: wrong socket in callback");
    }

    if (status != 0) { // just delete on error
        requests_.drop(1);
        return;
    } else if (n + m != 24) { // ensure valid packet
        throw runtime_error("memcache::recv_response: unexpected packet size");
    }

    // parse packet
    const memreq &req = requests_.dequeue_one();
    if (data != &req) {
        throw runtime_error(
          "memcache::recv_response: wrong response-request packet match");
    }

    // record measurement
    auto now = generator::clock::now();
    auto delta = now - req.start_ts;
    if (delta <= generator::duration(0)) {
        throw std::runtime_error(
          "memcache::recv_response: sample arrived before it was sent");
    }
    uint64_t service_us =
      chrono::duration_cast<generator::duration>(delta).count();
    req.cb(this, service_us, 0, req.measure);
}
