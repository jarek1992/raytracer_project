#pragma once

#include "hittable.hpp"

class material_instance : public hittable {
public:
	material_instance(shared_ptr<hittable> obj, shared_ptr<material> mat)
		: object(obj)
		, new_material(mat)
	{}

	virtual bool hit(
		const ray& r, interval ray_t, hit_record& rec) const override {
		//check collision with the underlying object
		if (!object->hit(r, ray_t, rec)) {
			return false;
		}

		rec.mat = new_material; //override material
		return true;
	}

	aabb bounding_box() const override {
		return object->bounding_box();
	}

private:
	shared_ptr<hittable> object; 
	shared_ptr<material> new_material;

};