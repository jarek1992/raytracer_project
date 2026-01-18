#pragma once

#include <vector>
#include "rtweekend.hpp"

class bloom_filter {
public:
	float threshold;
	float intensity;
	int blur_radius;

	bloom_filter(float _threshold = 1.0f, float _intensity = 0.3f, int _radius = 4)
		: threshold(_threshold)
		, intensity(_intensity)
		, blur_radius(_radius)
	{}

	void apply(std::vector<color>& buffer, int width, int height) const {
		if (intensity <= 0 || blur_radius <= 0) {
			return;
		}

		size_t size = buffer.size();
		std::vector<color> bright_buffer(size, color(0, 0, 0));
		//bright pass
		for (size_t i = 0; i < buffer.size(); ++i) {
			//use luminance for better results
			float luminance = static_cast<float>(buffer[i].x() * 0.2126 + buffer[i].y() * 0.7152 + buffer[i].z() * 0.0722);
			if (luminance > threshold) {
				bright_buffer[i] = buffer[i] * static_cast<double>(intensity);
			}
		}

		//blur pass (horizontal)
		std::vector<color> temp_buffer = bright_buffer;
		blur_pass(bright_buffer, temp_buffer, width, height, true);
		//blur pass(vertical)
		blur_pass(temp_buffer, bright_buffer, width, height, false);

		//combine original and bloom
		for (size_t i = 0; i < buffer.size(); ++i) {
			buffer[i] += bright_buffer[i];
		}
	}

private:
	void blur_pass(const std::vector<color>& input, std::vector<color>& output,
		int width, int height, bool horizontal) const {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				color sum(0.0, 0.0, 0.0);
				int count = 0;

				for (int offset = -blur_radius; offset <= blur_radius; ++offset) {
					int sample_x = x + (horizontal ? offset : 0);
					int sample_y = y + (horizontal ? 0 : offset);

					if (sample_x >= 0 && sample_x < width && sample_y >= 0 && sample_y < height) {
						sum += input[sample_y * width + sample_x];
						count++;
					}
				}
				output[y * width + x] = sum / static_cast<double>(count);
			}
		}
	}
};