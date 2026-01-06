#pragma once

#include "hittable.hpp"

class scale : public hittable {
public:
	scale(shared_ptr<hittable> object, const vec3& scale_factors)
		: object(object)
		, s(scale_factors)
	{
		//scale bounding box
		bbox = object->bounding_box();
		//scale min and max box - use 2 points
		point3 min_p(bbox.x.min * s.x(), bbox.y.min * s.y(), bbox.z.min * s.z());
		point3 max_p(bbox.x.max * s.x(), bbox.y.max * s.y(), bbox.z.max * s.z());

		bbox = aabb(min_p, max_p);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		point3 local_origin(r.origin().x() / s.x(), r.origin().y() / s.y(), r.origin().z() / s.z());
		vec3 local_dir(r.direction().x() / s.x(), r.direction().y() / s.y(), r.direction().z() / s.z());

		ray local_r(local_origin, local_dir, r.time());

		if (!object->hit(local_r, ray_t, rec))
			return false;

		//transform hit points back
		rec.p = point3(rec.p.x() * s.x(), rec.p.y() * s.y(), rec.p.z() * s.z());

		vec3 local_normal(rec.normal.x() / s.x(), rec.normal.y() / s.y(), rec.normal.z() / s.z());
		rec.normal = unit_vector(local_normal);

		return true;
	}

	aabb bounding_box() const override {
		return bbox;
	}

private:
	shared_ptr<hittable> object;
	vec3 s;
	aabb bbox;
};