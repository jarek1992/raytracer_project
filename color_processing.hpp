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
		}
		if (max_pixels > 0) {
			for (int i = 0; i < 256; i++) {
				normalized_histogram[i] = static_cast<float>(histogram[i]) / max_pixels;
			}
		}
	}
};

struct debug_flags {
	bool red = false;
	bool green = false;
	bool blue = false;
	bool luminance = false;

	//check if any debug mode is active
	bool any_active() const {
		return red || green || blue || luminance;
	}

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
	float exposure_compensation_stops = 0.0f; // 0.0 - no correction

	bool use_aces_tone_mapping = false;
	bool use_auto_exposure = false;
	float target_luminance = 0.22f; //aimed value for autoexposure (middle gray standard in photography 22%, higher value = overburn)

	//struct debug_flags instance
	mutable debug_flags debug;

	//bloom effect
	bool use_bloom = false;
	float bloom_threshold = 1.0f;
	float bloom_intensity = 0.3f;
	int bloom_radius = 4;

	bool needs_update = true;

	//hisogram datas for Imgui plot 
	mutable image_statistics last_stats;

	//post-denoise sharpening
	bool use_sharpening = false;
	double sharpen_amount = 0.2; //0.05 - 0.3 suggested range

	color process(color exposed_color, float u = 0.5f, float v = 0.5f) const {
		color c = exposed_color;
		//1. color balance(HDR)
		c = color(
			c.x() * color_balance.x(),
			c.y() * color_balance.y(),
			c.z() * color_balance.z());
		//2. contrast (0-1 range)
		if (std::abs(contrast - 1.0f) > 0.001f) {
			c = apply_contrast(c, contrast);
		}
		//8. vignette effect
		if (vignette_intensity > 0.0f) {
			float dist = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
			float vig = std::clamp(1.0f - dist * vignette_intensity, 0.0f, 1.0f);
			c *= static_cast<double>(vig);
		}
		//3. HSV operations
		if (std::abs(saturation - 1.0f) > 0.001f || std::abs(hue_shift) > 0.001f) {
			double original_luma = c.x() * 0.2126 + c.y() * 0.7152 + c.z() * 0.0722;

			color safe_hsv_input = color(
				std::clamp(c.x(), 0.0, 1.0),
				std::clamp(c.y(), 0.0, 1.0),
				std::clamp(c.z(), 0.0, 1.0)
			);
			vec3 hsv = rgb_to_hsv(safe_hsv_input);

			hsv[0] = std::fmod(hsv[0] + hue_shift, 360.0f);
			if (hsv[0] < 0) hsv[0] += 360.0f;
			hsv[1] = std::clamp(static_cast<float>(hsv[1] * saturation), 0.0f, 1.0f);

			color rgb_shifted = hsv_to_rgb(hsv);

			// Przywracamy energię HDR po operacjach HSV
			if (original_luma > 1.0) {
				c = rgb_shifted * original_luma;
			}
			else {
				c = rgb_shifted;
			}
		}
		//5. tone mapping apply_aces (0-1 range)
		if (use_aces_tone_mapping) {
			c = apply_aces(c);
		}

		//9. flags debug modes RGB/Luminance
		if (debug.any_active()) {
			c = apply_debug_view(c);
		}

		return linear_to_gamma(color(
			std::clamp(c.x(), 0.0, 1.0),
			std::clamp(c.y(), 0.0, 1.0),
			std::clamp(c.z(), 0.0, 1.0)
		));
	}

	//function to analyze framebuffer and compute image statistics
	image_statistics analyze_framebuffer(const std::vector<color>& framebuffer) const {
		image_statistics stats;
		double total_log_lum = 0.0;

		//define HDR range for histogram (e.i. 2^-10 to 2^10)
		const float min_log = -10.0f;
		const float max_log = 10.0f;
		const float log_range = max_log - min_log;

		for (const auto& pix : framebuffer) {
			float lum = static_cast<float>(pix.luminance());

			//general statistics
			if (lum > stats.max_luminance) {
				stats.max_luminance = lum;
			}

			//logarithmic mean (better for auto-exposure)
			float clamped_lum = std::max(0.0001f, lum);
			total_log_lum += std::log2(clamped_lum);

			//logarithmic histogram: map luminance to 0-255 range (brightness 0.0-1.0)
			float log_lum = std::log2(clamped_lum);
			float normalized_log = (log_lum - min_log) / log_range;
			int bin = std::clamp(static_cast<int>(normalized_log * 255.0f), 0, 255);

			stats.histogram[bin]++;
		}

		stats.average_luminance = std::pow(2.0f, static_cast<float>(total_log_lum / framebuffer.size()));
		stats.normalize();
		return stats;
	}

	//auto-exposure applied to final render
	double apply_auto_exposure(const image_statistics& stats) const {
		if (!use_auto_exposure) {
			return std::clamp(static_cast<float>(exposure), 0.01f, 20.0f);
		}

		double current_exp;
		// Jeśli obraz jest prawie czarny, używamy bardzo małej luminancji, 
		// żeby nie dzielić przez zero, ale wciąż stosujemy kompensację!
		double safe_luminance = std::max(static_cast<double>(stats.average_luminance), 0.0001);

		double raw_exposure = target_luminance / safe_luminance;
		current_exp = raw_exposure * std::pow(2.0, static_cast<double>(exposure_compensation_stops));

		return std::clamp(static_cast<float>(current_exp), 0.01f, 20.0f);
	}

	//post-denoise sharpness filter
	void apply_sharpening(std::vector<color>& buffer, int width, int height, double amount) const {
		if (amount <= 0.0) {
			return;
		}
		std::vector<color> original = buffer; //copy of original buffer

		for (size_t y = 1; y < static_cast<size_t>(height) - 1; ++y) {
			for (size_t x = 1; x < static_cast<size_t>(width) - 1; ++x) {
				size_t idx = y * width + x;

				//simple sharpenning filter (Laplacian kernel)
				//[ 0 -1  0]
				//[-1  5 -1]
				//[ 0 -1  0]

				color sum = original[idx] * 5.0;
				sum -= original[(y - 1) * width + x];
				sum -= original[(y + 1) * width + x];
				sum -= original[y * width + (x - 1)];
				sum -= original[y * width + (x + 1)];

				//mix original and sharpened based on amount (interpolation)
				buffer[idx] = (original[idx] * (1.0 - amount)) + (sum * amount);
			}
		}
	}

private:
	color apply_contrast(color c, float contrast) const {

		//get pixel brightness (luminance)
		double lum = c.luminance();
		//linear contrast 
		double new_lum = (lum - 0.5) * contrast + 0.5;
		new_lum = std::clamp(new_lum, 0.0, 1.0);

		if (lum > 0.0001) {
			return c * (new_lum / lum);
		}
		return color(new_lum, new_lum, new_lum);
	}

	color apply_debug_view(color c) const {
		//if luminance debug mode is active, overwrite rgb and display luminance view
		if(debug.luminance) {
			double lum = c.luminance();
			if (lum >= 1.0) {
				return color(1.0, 1.0, 1.0);
			}
			if (lum > 0.95) {
				return color(1.0, 0.0, 0.0);
			}
			if (lum > 0.70) {
				return color(1.0, 1.0, 0.0);
			}
			if (lum > 0.40) {
				return color(0.5, 0.5, 0.5);
			}
			if (lum > 0.10) {
				return color(0.0, 0.5, 0.0);
			}
			if (lum > 0.02) {
				return color(0.0, 0.0, 1.0);
			}
			else {
				return color(0.1, 0.0, 0.2);
			}
		}
		//rgb channels mix
		double r = debug.red ? c.x() : 0.0;
		double g = debug.green ? c.y() : 0.0;
		double b = debug.blue ? c.z() : 0.0;

		return color(r, g, b);
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

		s = max < 1e-6f ? 0.0f : d / max;

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