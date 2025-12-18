#pragma once

#include "rtweekend.hpp"

#include <limits>
#include <algorithm> // for std::fmin and std::fmax


class interval {
public:
	double min, max;

	// Default interval constructor is empty
	interval()
		: min(+std::numeric_limits<double>::infinity()) // Use std::numeric_limits to get infinity
		, max(-std::numeric_limits<double>::infinity()) // Use std::numeric_limits to get negative infinity
	{
	}

	// parametrical constructor with interval min and max
	interval(double min, double max) : min(min), max(max) {}

	// constructor combining two intervals
	interval(const interval& a, const interval& b) {
		min = std::fmin(a.min, b.min);
		max = std::fmax(a.max, b.max);
	}

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

//define the static constants for empty and universe intervals
const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

//interval translation by displacement
inline interval operator+(const interval& ival, double displacement) {
	return interval(ival.min + displacement, ival.max + displacement);
}

//number + interval
inline interval operator+(double displacement, const interval& ival) {
	return ival + displacement;
}