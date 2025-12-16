#pragma once

#include <cmath>
#include <random>
#include <iostream>
#include <limits>
#include <memory>

//c++ std usings
using std::make_shared;
using std::shared_ptr;

//constants
constexpr double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.14159265358979323846;
const double ray_epsilon = 0.0001;

//utility functions

//convert degrees to radians
inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

//generates a random double between 0 and 1
inline double random_double() {
	static std::random_device rd;  //random device
	static std::mt19937 gen(rd()); //mersenne Twister random number generator
	std::uniform_real_distribution<> dis(0.0, 1.0); //uniform distribution [0,1)
	return dis(gen);
}

//generates a random double between `min` and `max`
inline double random_double(double min, double max) {
	return min + (max - min) * random_double();
}

//common headers
#include "color.hpp"
#include "interval.hpp"
#include "ray.hpp"