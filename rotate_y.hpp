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
		bbox = ptr->bounding_box();

		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

					auto newx = cos_theta * x + sin_theta * z;
					auto newz = -sin_theta * x + cos_theta * z;

					vec3 tester(newx, y, newz);

					for (int c = 0; c < 3; c++) {
						min[c] = fmin(min[c], tester[c]);
						max[c] = fmax(max[c], tester[c]);
					}
				}
			}
		}
		bbox = aabb(min, max);
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

	aabb bounding_box() const override { return bbox; }

private:
	shared_ptr<hittable> ptr;
	double sin_theta;
	double cos_theta;
	aabb bbox;
};