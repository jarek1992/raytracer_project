#pragma once

#include "vec3.hpp"
#include "rtweekend.hpp"

#include <algorithm>
#include <cmath>

class post_processor {
public:
	double exposure = 1.0;
	double saturation = 1.0;
	double contrast = 1.0;
	double hue_shift = 0.0; //in degrees [-180, 180]
	double vignette_intensity = 1.0;
	vec3 color_balance = vec3(1.0, 1.0, 1.0); //RGB tint
	double z_depth_max_dist = 1.0; //for depth buffer normalization
	bool use_aces_tone_mapping = true;

	color process(color raw_color, double u = 0.5, double v = 0.5) const {
		color c = raw_color * exposure;
		//apply_aces
		if (use_aces_tone_mapping) {
			c = apply_aces(c);
		} else {
			c = color(std::clamp(c.x(), 0.0, 1.0),
				std::clamp(c.y(), 0.0, 1.0),
				std::clamp(c.z(), 0.0, 1.0));
		}

		c = apply_contrast(c, contrast);

		//HSV operations
		vec3 hsv = rgb_to_hsv(c);
		hsv[0] = std::fmod(hsv[0] + hue_shift, 360.0); //hue shift
		if (hsv[0] < 0) {
			hsv[0] += 360.0;
		}
		hsv[1] = std::clamp(hsv[1] * saturation, 0.0, 1.0); //saturation
		c = hsv_to_rgb(hsv);

		c = color(
			c.x() * color_balance.x(),
			c.y() * color_balance.y(),
			c.z() * color_balance.z()
		);

		//vignette effect
		if (vignette_intensity > 0.0) {
			double dist = std::sqrt((u - 0.5) * (u - 0.5) + (v - 0.5) * (v - 0.5));
			double vig = std::clamp(1.0 - dist * vignette_intensity, 0.0, 1.0);
			c *= vig;
		}

		return linear_to_gamma(c);
	}


private:
	color apply_contrast(color c, double contrast) const {
		auto midpoint = 0.5;
		return color(
			std::pow(std::clamp(c.x(), 0.0, 1.0) / midpoint, contrast) * midpoint,
			std::pow(std::clamp(c.y(), 0.0, 1.0) / midpoint, contrast) * midpoint,
			std::pow(std::clamp(c.z(), 0.0, 1.0) / midpoint, contrast) * midpoint
		);
	}

	vec3 rgb_to_hsv(vec3 c) const {
		double r = c.x(), g = c.y(), b = c.z();
		double max = std::max({ r, g, b }), min = std::min({ r, g, b });
		double h, s, v = max;
		double d = max - min;
		s = max == 0 ? 0 : d / max;
		if (max == min) h = 0;
		else {
			if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
			else if (max == g) h = (b - r) / d + 2;
			else if (max == b) h = (r - g) / d + 4;
			h /= 6;
		}
		return vec3(h * 360.0, s, v);
	}

	vec3 hsv_to_rgb(vec3 hsv) const {
		double h = hsv.x() / 360.0, s = hsv.y(), v = hsv.z();
		int i = int(h * 6);
		double f = h * 6 - i;
		double p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);

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
			return vec3(0, 0, 0);
		}
		}
	};
};