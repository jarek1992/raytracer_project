

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


private:
	std::map<std::string, std::shared_ptr<material>> library;
};