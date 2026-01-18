#pragma once

#include "vec3.hpp"
#include "rtweekend.hpp"

#include <algorithm>
#include <cmath>

struct image_statistics {
	float average_luminance = 0.0f;
	float max_luminance = 0.0f;
	int histogram[256] = { 0 }; //256 bins for luminance distribution
	float normalized_histogram[256] = { 0.0f }; //for plotting purposes Imgui

	void normalize() {
		int max_pixels = 0;
		for (int i = 0; i < 256; i++) {
			if (histogram[i] > max_pixels) {
				max_pixels = histogram[i];
			}
			if (max_pixels > 0) {
				for (int i = 0; i < 256; i++) {
					normalized_histogram[i] = static_cast<float>(histogram[i]) / max_pixels;
				}
			}
		}
	}
};

enum class debug_mode {
	NONE,
	RED,
	GREEN,
	BLUE,
	LUMINANCE
};

class post_processor {
public:
	mutable float exposure = 1.0f;
	float saturation = 1.0f;
	float contrast = 1.0f;
	float hue_shift = 0.0f; //in degrees [-180, 180]
	float vignette_intensity = 1.0f;
	vec3 color_balance = vec3(1.0f, 1.0f, 1.0f); //RGB tint
	float z_depth_max_dist = 1.0f; //for depth buffer normalization

	bool use_aces_tone_mapping = true;
	bool use_auto_exposure = false;
	float target_luminance = 0.22f; //aimed value for autoexposure (middle gray standard in photography 22%, higher value = overburn)
	mutable debug_mode current_debug_mode = debug_mode::NONE;

	//bloom effect
	bool use_bloom = false;
	float bloom_threshold = 1.0f;
	float bloom_intensity = 0.3f;
	int bloom_radius = 5;;

	color process(color raw_color, float u = 0.5f, float v = 0.5f) const {
		//1. exposure (double/HDR)
		color c = raw_color * static_cast<double>(exposure);
		//tone mapping apply_aces (0-1 range)
		if (use_aces_tone_mapping) {
			c = apply_aces(c);
		}
		//clamping before color operations 
		//
		//prevent from weird values like 1.5, -1 for constrast,hsv 
		c = color(
			std::clamp(static_cast<float>(c.x()), 0.0f, 1.0f),
			std::clamp(static_cast<float>(c.y()), 0.0f, 1.0f),
			std::clamp(static_cast<float>(c.z()), 0.0f, 1.0f)
		);
		//2. contrast (0-1 range)
		if (std::abs(contrast - 1.0f) > 0.001f) {
			c = apply_contrast(c, contrast);
		}
		//3. HSV operations
		vec3 hsv = rgb_to_hsv(c);
		//4. hue shift and saturation
		hsv[0] = std::fmod(hsv[0] + hue_shift, 360.0f); //hue shift
		if (hsv[0] < 0) {
			hsv[0] += 360.0f;
		}
		hsv[1] = std::clamp(static_cast<float>(hsv[1] * saturation), 0.0f, 1.0f); //saturation
		c = hsv_to_rgb(hsv);
		//5. color balance
		c = color(
			c.x() * color_balance.x(),
			c.y() * color_balance.y(),
			c.z() * color_balance.z());
		//6. vignette effect
		if (vignette_intensity > 0.0f) {
			float dist = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
			float vig = std::clamp(1.0f - dist * vignette_intensity, 0.0f, 1.0f);
			c *= static_cast<double>(vig);
		}
		//7. flags debug modes RGB/Luminance
		if (current_debug_mode != debug_mode::NONE) {
			double r = c.x();
			double g = c.y();
			double b = c.z();
			switch (current_debug_mode) {
				case debug_mode::RED: {
					c = color(r, 0.0, 0.0);
					break;
				}
				case debug_mode::GREEN: {
					c = color(0.0, g, 0.0);
					break;
				}
				case debug_mode::BLUE: {
					c = color(0.0, 0.0, b);
					break;
				}
				case debug_mode::LUMINANCE: {
					double lum = c.luminance();
					c = color(lum, lum, lum);
					break;
				}
				default:
					break;
				}
		}

		return linear_to_gamma(c);
	}

	//function to analyze framebuffer and compute image statistics
	image_statistics analyze_framebuffer(const std::vector<color>& framebuffer) const {
		image_statistics stats;
		double total_lum = 0.0;

		for (const auto& pix : framebuffer) {
			float lum = static_cast<float>(pix.luminance());
			if (lum > stats.max_luminance) {
				stats.max_luminance = lum;
			}
			total_lum += lum;
			//histogram: map luminance to 0-255 range (brightness 0.0-1.0)
			int bin = std::clamp(static_cast<int>(lum * 255.0f), 0, 255);
			stats.histogram[bin]++;
		}

		stats.average_luminance = static_cast<float>(total_lum / framebuffer.size());
		stats.normalize();
		return stats;
	}

	void apply_auto_exposure(const image_statistics& stats) const {
		if (use_auto_exposure && stats.average_luminance > 0.001f) {
			exposure = target_luminance / stats.average_luminance;
		}
		//clamp exposure to avoid over/under exposure
		exposure = std::clamp(exposure, 0.1f, 10.0f);
	}

private:
	color apply_contrast(color c, float contrast) const {
		auto midpoint = 0.5f;
		return color(
			std::pow(std::clamp(static_cast<float>(c.x()), 0.0f, 1.0f) / midpoint, contrast) * midpoint,
			std::pow(std::clamp(static_cast<float>(c.y()), 0.0f, 1.0f) / midpoint, contrast) * midpoint,
			std::pow(std::clamp(static_cast<float>(c.z()), 0.0f, 1.0f) / midpoint, contrast) * midpoint
		);
	}

	vec3 rgb_to_hsv(vec3 c) const {
		float r = static_cast<float>(c.x());
		float g = static_cast<float>(c.y());
		float b = static_cast<float>(c.z());

		float max = std::max({ r, g, b });
		float min = std::min({ r, g, b });

		float h = 0.0f;
		float s, v = max;
		float d = max - min;

		s = max == 0.0f ? 0.0f : d / max;

		if (max == min) {
			h = 0.0f;
		}
		else {
			if (max == r) {
				h = (g - b) / d + (g < b ? 6.0f : 0.0f);
			}
			else if (max == g) {
				h = (b - r) / d + 2.0f;
			}
			else if (max == b) {
				h = (r - g) / d + 4.0f;
			}
			h /= 6.0f;
		}
		return vec3(h * 360.0f, s, v);
	}

	vec3 hsv_to_rgb(vec3 hsv) const {
		float h = static_cast<float>(hsv.x()) / 360.0f;
		float s = static_cast<float>(hsv.y());
		float v = static_cast<float>(hsv.z());

		int i = static_cast<int>(h * 6.0f);
		float f = h * 6.0f - static_cast<float>(i);
		float p = v * (1.0f - s);
		float q = v * (1.0f - f * s);
		float t = v * (1.0f - (1.0f - f) * s);

		switch (i % 6) {
		case 0: {
			return vec3(v, t, p);
		}
		case 1: {
			return vec3(q, v, p);
		}
		case 2: {
			return vec3(p, v, t);
		}
		case 3: {
			return vec3(p, q, v);
		}
		case 4: {
			return vec3(t, p, v);
		}
		case 5: {
			return vec3(v, p, q);
		}
		default: {
			return vec3(0.0f, 0.0f, 0.0f);
		}
		}
	};
};