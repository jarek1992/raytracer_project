#pragma once

#include "interval.hpp"
#include "vec3.hpp"

#include <ostream>

using color = vec3;

//gamma correction
inline double linear_to_gamma(double linear_component) {
		return std::sqrt(linear_component);
}

// save pixel_color to the stream
void write_color(std::vector<unsigned char>& image, int idx, const color& pixel_color, int samples_per_pixel) {
	double scale = 1.0 / samples_per_pixel;

	// gamma correction
	double r = sqrt(pixel_color.x() * scale);
	double g = sqrt(pixel_color.y() * scale);
	double b = sqrt(pixel_color.z() * scale);

	//save to vector as 8-bits color
	image[idx + 0] = static_cast<unsigned char>(256 * std::clamp(r, 0.0, 0.999));
	image[idx + 1] = static_cast<unsigned char>(256 * std::clamp(g, 0.0, 0.999));
	image[idx + 2] = static_cast<unsigned char>(256 * std::clamp(b, 0.0, 0.999));
}