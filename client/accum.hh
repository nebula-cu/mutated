#ifndef ACCUM_HH
#define ACCUM_HH

/**
 * acumm.h - an accumulator utility
 */

#include <cstdint>
#include <vector>

class accum
{
private:
	std::vector<uint64_t> samples;

public:
	using size_type = std::vector<uint64_t>::size_type;

	accum(void) : samples{} {}

	void clear(void);
	void add_sample(uint64_t val);

	double mean(void);
	double stddev(void);
	uint64_t percentile(double percent);
	uint64_t min(void);
	uint64_t max(void);
	size_type size(void);
};

#endif /* ACCUM_HH */
