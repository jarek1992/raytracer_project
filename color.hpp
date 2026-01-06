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

	//get raw components after averaging the samples
	double r_raw = pixel_color.x() * scale;
	double g_raw = pixel_color.y() * scale;
	double b_raw = pixel_color.z() * scale;

	//tone mapping
	r_raw = r_raw / (1.0 + r_raw);
	g_raw = g_raw / (1.0 + r_raw);
	b_raw = b_raw / (1.0 + r_raw);

	//gamma correction
	double r = sqrt(r_raw);
	double g = sqrt(r_raw);
	double b = sqrt(r_raw);


	//save to vector as 8-bits color
	image[idx + 0] = static_cast<unsigned char>(256 * std::clamp(r, 0.0, 0.999));
	image[idx + 1] = static_cast<unsigned char>(256 * std::clamp(g, 0.0, 0.999));
	image[idx + 2] = static_cast<unsigned char>(256 * std::clamp(b, 0.0, 0.999));
}