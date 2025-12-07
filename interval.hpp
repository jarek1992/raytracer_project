#pragma once

#include <limits>

class interval {
public:
	double min, max;

	// Default interval constructor is empty
	interval()
		: min(+std::numeric_limits<double>::infinity()) // Use std::numeric_limits to get infinity
		, max(-std::numeric_limits<double>::infinity()) // Use std::numeric_limits to get negative infinity
	{}

	// parametrical constructor with interval min and max
	interval(double min, double max) : min(min), max(max) {}

	// return size of interval
	double size() const { return max - min; }
	// check if interval contains x
	bool contains(double x) const { return min <= x && x <= max; }
	bool surrounds(double x) const { return min < x && max > x; }
	double clamp(double x) const {
		if (x < min) { return min; }
		if (x > max) { return max; }
		return x;
	}
	static const interval empty, universe;
};

// Define the static constants for empty and universe intervals
const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);