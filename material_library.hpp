

#pragma once

#include "material.hpp"

#include <map>
#include <string>
#include <memory>
#include <vector>

//materials library
class MaterialLibrary {
public:
	MaterialLibrary() = default;

	//add material to library
	void add(const std::string& name, std::shared_ptr<material> m) {
		library[name] = m;
	}

	//get material from library by name
	std::shared_ptr<material> get(const std::string& name) const {
		auto it = library.find(name);
		if (it != library.end()) {
			return it->second;
		}
		//if material not found, return 
		std::cerr << "Material '" << name << "' not found in library. Returning nullptr." << std::endl;
		return nullptr;
	}

	//get all material names in the library
	std::vector<std::string> get_material_names() const {
		std::vector<std::string> names;
		for (const auto& [name, mat] : library) {
			names.push_back(name);
		}
		return names;
	}

	//FILTER METHODS
	//get only emissive materials
	std::vector<std::string> get_emissive_names() const {
		std::vector<std::string> emissive;
		for (const auto& [name, mat] : library) {
			if (name.find("neon") != std::string::npos || name.find("emissive") != std::string::npos) {
				emissive.push_back(name);
			}
		}
		return emissive;
	}

	//get all the materials which are non emissive and floor material
	std::vector<std::string> get_regular_names() const {
		std::vector<std::string> regular;
		for (const auto& [name, mat] : library) {
			bool is_light = (name.find("neon") != std::string::npos || name.find("emissive") != std::string::npos);
			bool is_ground = (name == "reflective_checker_mat");

			if (!is_light && !is_ground) {
				regular.push_back(name);
			}
		}
		return regular;
	}

private:
	std::map<std::string, std::shared_ptr<material>> library;
};