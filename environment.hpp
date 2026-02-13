#pragma once

#include "vec3.hpp"
#include "texture.hpp"

inline const std::string HDR_DIR = "assets/hdr_maps/";

struct EnvironmentSettings {
	enum Mode {
		PHYSICAL_SUN,
		HDR_MAP,
		SOLID_COLOR
	};
	Mode mode = PHYSICAL_SUN;

	color background_color = color(0.0, 0.0, 0.0);

	std::string current_hdr_name = "None"; //to display in UI
	std::string current_hdr_path = HDR_DIR; //folder path to hdr maps

	//GENERAL settings
	double intensity = 1.0; // overall intensity multiplier

	//HDRI map rotation settings
	double hdri_rotation = 0.0; //yaw rotation in radians
	double hdri_tilt = 0.0; //pitch/tilt rotation in radians
	double hdri_roll = 0.0; //roll rotation in radians
	
	shared_ptr<image_texture> hdr_texture = nullptr;

	//loading hdr maps function
	void load_hdr(const std::string& path) {
		if (path.empty()) {
			this->mode = SOLID_COLOR;
			this->background_color = color(0.0, 0.0, 0.0); //black
			this->current_hdr_name = "None (Black)";
			return;
		}

		try {
			hdr_texture = make_shared<image_texture>(path.c_str(), true);
			//get map name
			size_t last_slash = path.find_last_of("/\\");
			current_hdr_name = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
			current_hdr_path = path;

			//after succesful loading switch mode to HDR
			mode = HDR_MAP;
		}
		catch (...) {
			this->mode = SOLID_COLOR;
			this->background_color = color(0.0, 0.0, 0.0);
			std::cerr << "[Error] Could not load HDR. Falling back to black.\n";
		}
	}

	//physical sun settings
	vec3 sun_direction = unit_vector(vec3(1.0, 0.5, -0.5));
	color sun_color = color(1.0, 1.0, 1.0);
	bool auto_sun_color = true;
	double sun_intensity = 1.0;
	double sun_size = 1;
};