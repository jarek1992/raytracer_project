#pragma once

#include "hittable.hpp"
#include "vec3.hpp"

class translate : public hittable {
public:
	translate(shared_ptr<hittable> p, const vec3& displacement)
		: ptr(p)
		, offset(displacement)
	{
	}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

		//world -> local(move ray in opposite direction of offset)
		ray moved_r(
			r.origin() - offset, 
			r.direction()
		);

		if (!ptr->hit(moved_r, ray_t, rec))
			return false;
		//local -> world(move intersection point back)
		rec.p += offset;
		//normal remains the same(no rotation applied)
		rec.set_face_normal(r, rec.normal);

		return true;
	}

private:
	shared_ptr<hittable> ptr;
	vec3 offset;	

};