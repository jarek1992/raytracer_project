#pragma once

#include "vec3.hpp"
#include "texture.hpp"

struct EnvironmentSettings {
	enum Mode {
		PHYSICAL_SUN,
		HDR_MAP
	};
	Mode mode = PHYSICAL_SUN;

	//GENERAL settings
	double intensity = 1.0; // overall intensity multiplier

	//HDRI map rotation settings
	double hdri_rotation = 0.0; //yaw rotation in radians
	double hdri_tilt = 0.0;     //tilt rotation in radians
	shared_ptr<image_texture> hdr_texture = nullptr;

	//physical sun settings
	vec3 sun_direction = unit_vector(vec3(1.0, 0.5, -0.5));
	color sun_color = color(1.0, 0.9, 0.8);
	double sun_intensity = 5.0;
	double sun_size = 100; //higher values = smaller sun disk
};