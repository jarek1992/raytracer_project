#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <random>
#include <algorithm>

//c++ std usings
using std::make_shared;
using std::shared_ptr;

//constants
constexpr double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.14159265358979323846;
const double ray_epsilon = 0.0001;

//utility functions
//
//convert degrees to radians
inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

//convert radians to degrees
inline double radians_to_degrees(double radians) {
	return radians * 180.0 / pi;
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

//return random integer in [min,max]
inline int random_int(int min, int max) {
	return static_cast<int>(random_double(min, max + 1));
}

#include "vec3.hpp"

inline color apply_aces(color x) {
	//Pancerne zabezpieczenie przed ujemnymi wartościami (NaN killer)
	auto safe_x = [](double v) {
		// Zabezpieczenie przed NaN i nieskończonością
		if (std::isnan(v) || std::isinf(v)) {
			return 0.0; 
		} 
		double val = std::max(0.0, v); // Nigdy poniżej zera

		const double a = 2.51;
		const double b = 0.03;
		const double c = 2.43;
		const double d = 0.59;
		const double e = 0.14;

		return (val * (a * val + b)) / (val * (c * val + d) + e);
	};

	return color(safe_x(x.x()), safe_x(x.y()), safe_x(x.z()));
}

//convert linear color component to gamma corrected component
inline double linear_to_gamma(double linear_component) {
	if (linear_component > 0) {
		return std::pow(linear_component, 1.0 / 2.2);
	}
	return 0;
}

//convert linear color to gamma corrected color
inline color linear_to_gamma(color linear_color) {
	return color(
		linear_to_gamma(linear_color.x()),
		linear_to_gamma(linear_color.y()),
		linear_to_gamma(linear_color.z())
	);
}

//antyaliasing edges
inline double smoothstep(double edge0, double edge1, double x) {
	//scaling, clamping and calculating 3x^2 - 2x^3
	x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return x * x * (3 - 2 * x);
}

//convert spherical coordinates(degrees) to directional vectors
inline vec3 direction_from_spherical(double elevation_deg, double azimuth_deg) {
	double phi = degrees_to_radians(azimuth_deg);
	double theta = degrees_to_radians(90.0 - elevation_deg); //90.0 degrees is zenith (top)

	return vec3(
		std::sin(theta) * std::cos(phi),
		std::cos(theta),
		std::sin(theta) * std::sin(phi)
	);
}

inline bool is_nan(const color& c) {
	return std::isnan(c.x()) || std::isnan(c.y()) || std::isnan(c.z());
}

enum class render_pass : int {
	RGB = 0,
	DENOISE,
	ALBEDO,
	NORMALS,
	Z_DEPTH,
	REFLECTIONS,
	REFRACTIONS
};

#include "color.hpp"
#include "interval.hpp"
#include "ray.hpp"