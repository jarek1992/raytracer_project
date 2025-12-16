# pragma once 

#include "hittable.hpp"
#include "vec3.hpp"
#include <cmath>

class rotate_y : public hittable {
public:
	rotate_y(shared_ptr<hittable> p, double angle_rad)
		: ptr(p)
	{
		sin_theta = std::sin(angle_rad);
		cos_theta = std::cos(angle_rad);
	}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

		//World -> local
		point3 origin = r.origin();
		vec3 dir = r.direction();

		origin[0] = cos_theta * r.origin()[0] + sin_theta * r.origin()[2];
		origin[2] = -sin_theta * r.origin()[0] + cos_theta * r.origin()[2]; 

		dir[0] = cos_theta * r.direction()[0] + sin_theta * r.direction()[2]; 
		dir[2] = -sin_theta * r.direction()[0] + cos_theta * r.direction()[2]; 

		ray rotated_r(origin, dir);

		if (!ptr->hit(rotated_r, ray_t, rec))
			return false;

		//local -> world(rotate back the hit point and normal +sin_theta)
		point3 p = rec.p;
		vec3 normal = rec.normal;

		//transform P_Y(+theta)
		p[0] = cos_theta * rec.p[0] - sin_theta * rec.p[2];
		p[2] = sin_theta * rec.p[0] + cos_theta * rec.p[2];

		normal[0] = cos_theta * rec.normal[0] - sin_theta * rec.normal[2];
		normal[2] = sin_theta * rec.normal[0] + cos_theta * rec.normal[2];

		rec.p = p;
		rec.set_face_normal(r, normal);

		return true;
	}

private:
	shared_ptr<hittable> ptr;
	double sin_theta;
	double cos_theta;
};