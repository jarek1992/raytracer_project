#pragma once

#include "vec3.hpp"
#include "rtweekend.hpp"

#include <algorithm>
#include <cmath>

class post_processor {
public:
	float exposure = 1.0f;
	float saturation = 1.0f;
	float contrast = 1.0f;
	float hue_shift = 0.0f; //in degrees [-180, 180]
	float vignette_intensity = 1.0f;
	vec3 color_balance = vec3(1.0f, 1.0f, 1.0f); //RGB tint
	float z_depth_max_dist = 1.0f; //for depth buffer normalization
	bool use_aces_tone_mapping = true;

	color process(color raw_color, float u = 0.5f, float v = 0.5f) const {
		//exposure (double/HDR)
		color c = raw_color * static_cast<double>(exposure);
		//tone mapping apply_aces (0-1 range)
		if (use_aces_tone_mapping) {
			c = apply_aces(c);
		}
		//clamping before color operations 
		//prevent from weird values like 1.5, -1 for constrast,hsv 
		c = color(
			std::clamp(static_cast<float>(c.x()), 0.0f, 1.0f),
			std::clamp(static_cast<float>(c.y()), 0.0f, 1.0f),
			std::clamp(static_cast<float>(c.z()), 0.0f, 1.0f)
		);

		//contrast (0-1 range)
		if (std::abs(contrast - 1.0f) > 0.001f) {
			c = apply_contrast(c, contrast);
		}

		//HSV operations
		vec3 hsv = rgb_to_hsv(c);

		//hue shift and saturation
		hsv[0] = std::fmod(hsv[0] + hue_shift, 360.0f); //hue shift
		if (hsv[0] < 0) {
			hsv[0] += 360.0f;
		}
		hsv[1] = std::clamp(static_cast<float>(hsv[1] * saturation), 0.0f, 1.0f); //saturation
		
		c = hsv_to_rgb(hsv);

		//color balance
		c = color(
			c.x() * color_balance.x(),
			c.y() * color_balance.y(),
			c.z() * color_balance.z()
		);

		//vignette effect
		if (vignette_intensity > 0.0f) {
			float dist = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
			float vig = std::clamp(1.0f - dist * vignette_intensity, 0.0f, 1.0f);
			c *= static_cast<double>(vig);
		}

		return linear_to_gamma(c);
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