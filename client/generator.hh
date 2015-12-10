#ifndef GENERATOR_HH
#define GENERATOR_HH

/**
 * generator.h - support for load generators
 */

#include <cstdint>
#include <random>

#include "opts.hh"

class generator
{
public:
	/* Constructor */
	generator(void) {}

	/* No copy or move */
	generator(const generator &) = delete;
	generator(generator &&) = delete;
	generator & operator=(const generator &) = delete;
	generator & operator=(generator &&) = delete;

	/* Generate requests */
	void start(void) {};
	int do_request(bool should_measure);
};

#endif /* GENERATOR_HH */
