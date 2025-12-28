# pragma once 

#include "hittable.hpp"
#include "vec3.hpp"
#include <cmath>

class rotate_x : public hittable {
public:
	rotate_x(shared_ptr<hittable> p, double angle) : ptr(p) {
		auto radians = degrees_to_radians(angle);
		sin_theta = sin(radians);
		cos_theta = cos(radians);
		bbox = ptr->bounding_box();

		point3 min(infinity, infinity, infinity);
		point3 max(-infinity, -infinity, -infinity);

		//calculate new bounding box after rotation
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {
					auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
					auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
					auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

					//rotate around X axis
					auto newy = cos_theta * y - sin_theta * z;
					auto newz = sin_theta * y + cos_theta * z;

					vec3 tester(x, newy, newz);

					for (int c = 0; c < 3; c++) {
						min[c] = fmin(min[c], tester[c]);
						max[c] = fmax(max[c], tester[c]);
					}
				}
			}
		}
		bbox = aabb(min, max);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		//transform ray from world to local object space
		auto origin = r.origin();
		auto direction = r.direction();

		origin[1] = cos_theta * r.origin()[1] + sin_theta * r.origin()[2];
		origin[2] = -sin_theta * r.origin()[1] + cos_theta * r.origin()[2];

		direction[1] = cos_theta * r.direction()[1] + sin_theta * r.direction()[2];
		direction[2] = -sin_theta * r.direction()[1] + cos_theta * r.direction()[2];

		ray rotated_r(origin, direction, r.time());

		if (!ptr->hit(rotated_r, ray_t, rec)) return false;

		//transform hit point and normal back to world space
		auto p = rec.p;
		p[1] = cos_theta * rec.p[1] - sin_theta * rec.p[2];
		p[2] = sin_theta * rec.p[1] + cos_theta * rec.p[2];

		auto normal = rec.normal;
		normal[1] = cos_theta * rec.normal[1] - sin_theta * rec.normal[2];
		normal[2] = sin_theta * rec.normal[1] + cos_theta * rec.normal[2];

		rec.p = p;
		rec.normal = normal;

		return true;
	}

	aabb bounding_box() const override { return bbox; }

private:
	shared_ptr<hittable> ptr;
	double sin_theta;
	double cos_theta;
	aabb bbox;
};