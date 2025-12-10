#pragma once

#include <vector>
#include <cmath>
#include <string>

#include "vec3.hpp"
#include "stb_image.h" 


inline double HDRI_ROTATION = 0.0; // rotation in radians

// Clamp helper
inline int clamp_int(int x, int a, int b) {
	return std::max(a, std::min(b, x));
}

class hdr_image {
public:
	int width = 0;
	int height = 0;
	std::vector<vec3> data;

	//loading HDR file using stb_image
	bool load(const std::string& filename) {
		float* pixels = stbi_loadf(filename.c_str(), &width, &height, nullptr, 3);
		if (!pixels) {
			std::cerr << "Failed to load HDR image: " << filename << "\n";
			return false;
		}

		data.resize(width * height);

		for (int i = 0; i < width * height; i++) {
			data[i] = vec3(
				pixels[i * 3 + 0],
				pixels[i * 3 + 1],
				pixels[i * 3 + 2]
			);
		}

		stbi_image_free(pixels);
		return true;
	}

	//sample pixel (u,v in [0..1])
	vec3 sample(double u, double v) const {
		if (width == 0 || height == 0) {
			return vec3(0, 0, 0);
		}

		//wrap around
		u = u - std::floor(u);
		v = v - std::floor(v);

		int x = clamp_int(int(u * width), 0, width - 1);
		int y = clamp_int(int(v * height), 0, height - 1);

		return data[y * width + x];
	}

	//converts ray direction → spherical coords → HDR sample
	vec3 environment(const vec3& d) const {
		constexpr double pi = 3.14159265358979323846;

		vec3 nd = unit_vector(d);

		double theta = acos(std::clamp(nd.y(), -1.0, 1.0));
		double phi = atan2(nd.z(), nd.x()) + pi;

		phi += pi;
		phi += HDRI_ROTATION;

		double u = phi / (2.0 * pi);
		double v = theta / pi;

		// STB LOADS HDRI UPSIDE DOWN → fix
		//v = 1.0 - v;*/

		return sample(u, v);
	}
};

inline hdr_image hdri_image;