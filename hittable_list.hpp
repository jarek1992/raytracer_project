#pragma once

#include "hittable.hpp"

#include <vector>

class hittable_list : public hittable {
public:
	std::vector<std::shared_ptr<hittable>> objects;

	//default constructor
	hittable_list() {}
	//constructor with one argument added to the list 
	hittable_list(std::shared_ptr<hittable> object) { 
		add(object); 
	}
	//remove objects from the list
	void clear() { 
		objects.clear(); 
	}
	//add new object to the list
	void add(std::shared_ptr<hittable> object) {
		objects.push_back(object);
		bbox = aabb(bbox, object->bounding_box());
	}

	//go through the objects on the list to check if any ray hits them(objects)
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		hit_record temp_rec;
		bool hit_anything = false;
		auto closest_so_far = ray_t.max;

		for (const auto& object : objects) {
			if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				rec = temp_rec;
			}
		}
		return hit_anything;
	}

	aabb bounding_box() const override { return bbox; }

private:
	aabb bbox;
};