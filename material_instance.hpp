#pragma once

#include "hittable.hpp"

class material_instance : public hittable {
public:
	material_instance(shared_ptr<hittable> obj, shared_ptr<material> mat)
		: object(obj)
		, new_material(mat)
	{}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		//check collision with the underlying object
		if (!object->hit(r, ray_t, rec)) {
			return false;
		}

		//for safety , in case new_material is nullptr
		if (new_material != nullptr) {
			rec.mat = new_material;
		}
		else {

			//if material is missing, assign a bright error material
			static auto error_mat = make_shared<lambertian>(color(1, 0, 1));
			rec.mat = error_mat;
		}

		return true;
	}

	aabb bounding_box() const override {
		return object->bounding_box();
	}

	void set_material(shared_ptr<material> m) {
		new_material = m;
	}

private:
	shared_ptr<hittable> object;
	shared_ptr<material> new_material;

};