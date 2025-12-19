#pragma once

#include "rtweekend.hpp"
#include "libs/stb/stb_image.h"

class texture {
public:
	virtual ~texture() = default;
	virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
public:
	solid_color(const color& albedo)
		: albedo(albedo)
	{
	}
	solid_color(double red, double green, double blue)
		: solid_color(color(red, green, blue))
	{
	}

	color value(double u, double v, const point3& p) const override {
		return albedo;
	}

private:
	color albedo;
};

class image_texture : public texture {
public:
	image_texture(const char* filename) {
		auto components_per_pixel = bytes_per_pixel;
		data = stbi_load(
			filename, &width, &height, &components_per_pixel, components_per_pixel
		);

		if (!data) {
			std::cerr << "ERROR: Could not load texture image file '" << filename << "'.\n";
			width = height = 0;
		}
	}

	~image_texture() {
		if (data) {
			stbi_image_free(data);
		}
	}

	color value(double u, double v, const point3& p) const override {
		//if no texture data, return solid cyan as debugging aid
		if (height <= 0) {
			return color(1, 0, 1);
		}

		//clamp u,v to [0,1]
		u = interval(0, 1).clamp(u);
		v = 1.0 - interval(0, 1).clamp(v); //flip v to image coordinates

		auto i = int(u * width);
		auto j = int(v * height);

		//clamp integer mapping
		if (i >= width) {
			i = width - 1;
		}
		if (j >= height) {
			j = height - 1;
		}

		const auto color_scale = 1.0 / 255.0;
		auto pixel = data + (j * width + i) * bytes_per_pixel + i * bytes_per_pixel;
		return color(
			color_scale * pixel[0],
			color_scale * pixel[1],
			color_scale * pixel[2]
		);
	}

private:
	unsigned char* data;
	int width, height;
	const int bytes_per_pixel = 3;

};