#pragma once

#include "rtweekend.hpp"
#include "color_processing.hpp"

#include <vector>

// save pixel_color to the stream
void write_color(
	std::vector<unsigned char>& image, 
	int idx, 
	const color& pixel_color,
	int samples_per_pixel,
	post_processor& post,
	double u,
	double v
) {
	double scale = 1.0 / samples_per_pixel;

	//average from samples(linear color)
	color raw_linear_color(
		pixel_color.x() * scale,
		pixel_color.y() * scale,
		pixel_color.z() * scale
	);

	//exposure, ACES, hsv, contrast, vignette, gamma
	color final_color = post.process(raw_linear_color, u, v);;

	//save to vector as 8-bits color
	image[idx + 0] = static_cast<unsigned char>(255.999 * final_color.x());
	image[idx + 1] = static_cast<unsigned char>(255.999 * final_color.y());
	image[idx + 2] = static_cast<unsigned char>(255.999 * final_color.z());
}