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

	void generate_bloom_overlay(const std::vector<color>& render_accumulator,
		std::vector<color>& bloom_overlay,
		int width, 
		int height,
		float exposure) const {

		size_t size = render_accumulator.size();
		std::vector<color> bright_buffer(size, color(0.0, 0.0, 0.0));

		//bright pass
		for (size_t i = 0; i < size; ++i) {
			//multiply by exposure directly here
			color exposed_c = render_accumulator[i] * exposure;
			float lum = static_cast<float>(exposed_c.luminance());

			if (lum > threshold) {
				// Zmieniony wzór: zachowujemy kolor, ale skalujemy go intensywnością
				// Możesz też użyć prostego: bright_buffer[i] = exposed_c * intensity;
				float factor = (lum - threshold) * intensity;
				bright_buffer[i] = exposed_c * static_cast<double>(factor / std::max(lum, 0.0001f));
			}
		}

		//blur pass (horizontal)
		std::vector<color> temp_buffer(size);
		blur_pass(bright_buffer, temp_buffer, width, height, true);
		//blur pass(vertical)
		blur_pass(temp_buffer, bloom_overlay, width, height, false);
	}

private:
	void blur_pass(const std::vector<color>& input, std::vector<color>& output,
		int width, int height, bool horizontal) const {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				color sum(0.0, 0.0, 0.0);
				float total_weight = 0.0f;

				for (int offset = -blur_radius; offset <= blur_radius; ++offset) {
					int sample_x = x + (horizontal ? offset : 0);
					int sample_y = y + (horizontal ? 0 : offset);

					if (sample_x >= 0 && sample_x < width && sample_y >= 0 && sample_y < height) {
						//linear falloff better than box blur
						float weight = 1.0f - (std::abs(offset) / static_cast<float>(blur_radius + 1));
						sum += input[sample_y * width + sample_x] * static_cast<double>(weight);
						total_weight += weight;
					}
				}
				output[y * width + x] = (total_weight > 0) ? (sum / static_cast<double>(total_weight)) : color(0, 0, 0);
			}
		}
	}
};