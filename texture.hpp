#pragma once

#include "rtweekend.hpp"
#include "stb_image.h"

class texture {
public:
	virtual ~texture() = default;
	virtual color value(double u, double v, const point3& p) const = 0;
};

class image_texture : public texture {
public:
	image_texture(const char* filename, bool is_hdr = false) {
		int components = 3;
		if (is_hdr) {
			//loading data as a float (32-bit per canal)
			data_f = stbi_loadf(filename, &width, &height, &components, 3);
			if (!data_f) {
				std::cerr << "ERROR: Could not load HDR: " << filename << "\n";
			}
		} else {
			//loading data as unsigned char (8-bit per canal)
			data_u = stbi_load(filename, &width, &height, &components, 3);

			if (!data_u) {
				std::cerr << "ERROR: Could not load texture: " << filename << "\n";
			}
		}
	}

	~image_texture() {
		if (data_f) {
			stbi_image_free(data_f);
		}
		if (data_u) {
			stbi_image_free(data_u);
		}
	}

	color value(double u, double v, const point3& p) const override {
		//if no texture data, return solid cyan as debugging aid
		if (width <= 0 || height <= 0) {
			return color(0, 1, 1); //magenta debug color
		}

		//clamp and UV wrap
		u = u - std::floor(u);

		//stb loading HDRs upside down
		//v = 1.0 - (v - std::floor(v)); 

		int i = static_cast<int>(u * width);
		int j = static_cast<int>(v * height);

		//clamp integer mapping
		if (i >= width) {
			i = width - 1;
		}
		if (j >= height) {
			j = height - 1;
		}

		if (data_f) {
			auto pixel = data_f + j * width * 3 + i * 3;
			return color(pixel[0], pixel[1], pixel[2]);
		}
		else if (data_u) {
			const double scale = 1.0 / 255.0;
			auto pixel = data_u + j * width * 3 + i * 3;
			return color(scale * pixel[0], scale * pixel[1], scale * pixel[2]);
		}

		return color(0, 0, 0);
	}

private:
	float* data_f = nullptr;
	unsigned char* data_u = nullptr;
	int width, height;
};

class solid_color : public texture {
public:
	solid_color(const color& albedo)
		: albedo(albedo)
	{}
	solid_color(double red, double green, double blue)
		: solid_color(color(red, green, blue))
	{}

	color value(double u, double v, const point3& p) const override {
		return albedo;
	}

private:
	color albedo;
};

class checker_texture : public texture {
public:
	checker_texture(double scale, shared_ptr<texture> odd, shared_ptr<texture> even)
		: inv_scale(1.0 / scale)
		, odd(odd)
		, even(even)
	{}

	checker_texture(double scale, color c1, color c2)
		: inv_scale(1.0 / scale)
		, odd(make_shared<solid_color>(c1))
		, even(make_shared<solid_color>(c2))
	{}

	color value(double u, double v, const point3& p) const override {
		auto xInteger = static_cast<int>(std::floor(inv_scale * p.x()));
		auto yInteger = static_cast<int>(std::floor(inv_scale * p.y()));
		auto zInteger = static_cast<int>(std::floor(inv_scale * p.z()));

		bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

		return isEven ? even->value(u, v, p) : odd->value(u, v, p);
	}

private:
	double inv_scale;
	shared_ptr<texture> odd;
	shared_ptr<texture> even;

};