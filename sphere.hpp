#pragma once

#include "hittable.hpp"

class sphere : public hittable {
public:
	sphere(const point3& center, double radius, shared_ptr<material> mat)
		: center(center)
		, radius(std::fmax(0, radius)) //prevent from negative radius
		, mat(mat)
	{
		//calculate aabb box in constructor
		vec3 r_vec(radius, radius, radius);
		bbox = aabb(center - r_vec, center + r_vec);
	}

	//sphere
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		vec3 oc = center - r.origin();
		//ray-sphere intersection logic
		auto a = r.direction().length_squared();
		auto h = dot(r.direction(), oc);
		auto c = oc.length_squared() - radius * radius;
		auto discriminant = h * h - a * c;

		//check if radius hits the sphere
		if (discriminant < 0) {
			return false; //no intersections
		}
		auto sqrtd = std::sqrt(discriminant);

		//find the nearest root that lies in the acceptable range
		auto root = (h - sqrtd) / a;
		if (!ray_t.surrounds(root)) {
			root = (h + sqrtd) / a;
			if (!ray_t.surrounds(root)) {
				return false;
			}
		}
		//save the record
		rec.t = root;
		rec.p = r.at(rec.t);
		vec3 outward_normal = (rec.p - center) / radius;
		rec.set_face_normal(r, outward_normal);
		get_sphere_uv(outward_normal, rec.u, rec.v);
		rec.mat = mat;

		return true;
	}
	
	aabb bounding_box() const override {
		return bbox;
	}

	static void get_sphere_uv(const point3& p, double& u, double& v) {
		//p: a given point on the sphere of radius one, centered at the origin
		//u: returned value [0,1] of angle around the Y axis from X=-1
		//v: returned value [0,1] of angle from Y=-1 to Y=+1
		auto theta = acos(-p.y());
		auto phi = atan2(-p.z(), p.x()) + pi;

		u = phi / (2 * pi);
		v = theta / pi;
	}

	void set_material(std::shared_ptr<material> m) {
		mat = m;
	}

private:
	point3 center; //sphere center
	double radius; //sphere radius
	shared_ptr<material> mat; //material pointer
	aabb bbox; //bounding box
};