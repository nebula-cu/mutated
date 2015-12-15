#include <cmath>
#include <iostream>

#include <unistd.h>

#include "opts.hh"

using namespace std;

/* Microseconds in a second. */
static constexpr double USEC = 1000000;

/* Fixed arguments required */
static constexpr size_t FIXED_ARGS = 2;

/**
 * Print usage message and exit with status.
 */
static void __printUsage(string prog, int status = EXIT_FAILURE)
{
	if (status != EXIT_SUCCESS) {
		cerr << "invalid arguments!" << endl << endl;
	}

	cerr << "usage: " << prog
			 << " [-h] [-m] [-w integer] [-s integer] [-c integer] "
						"ip:port service_mean_us" << endl;
	cerr << "  -h: help" << endl;
	cerr << "  -m: machine-readable" << endl;
	cerr << "  -w: warm-up sample count" << endl;
	cerr << "  -s: measurement sample count" << endl;
	cerr << "  -c: cool-down sample count" << endl;
	cerr << "  -l: label for machine-readable output (-m)" << endl;

	exit(status);
}

/**
 * Parse command line.
 */
Config::Config(int argc, char *argv[])
	: port{0}, label{"default"}, service_us{0}, req_s{0}
	, pre_samples{100}, samples{1000}, post_samples{100}
	, total_samples{pre_samples + samples + post_samples}
	, machine_readable{false}
{
	int ret, c;
	opterr = 0;

	while ((c = getopt(argc, argv, "hbmw:s:c:l:n:")) != -1) {
		switch (c) {
		case 'h':
			__printUsage(argv[0], EXIT_SUCCESS);
		case 'm':
			machine_readable = true;
			break;
		case 'w':
			pre_samples = atoi(optarg);
			break;
		case 's':
			samples = atoi(optarg);
			break;
		case 'c':
			post_samples = atoi(optarg);
			break;
		case 'l':
			label = optarg;
			break;
		default:
			__printUsage(argv[0]);
		}
	}

	if (argc - optind != FIXED_ARGS) {
		__printUsage(argv[0]);
	}

	ret = sscanf(argv[optind+0], "%[^:]:%hu", addr, &port);
	if (ret != 2) {
		__printUsage(argv[0]);
	}

	ret = sscanf(argv[optind+1], "%lf", &service_us);
	if (ret != 1) {
		__printUsage(argv[0]);
	}

	req_s = USEC / service_us;
	total_samples = pre_samples + samples + post_samples;
}
