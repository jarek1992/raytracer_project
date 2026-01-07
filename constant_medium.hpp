#pragma once

#include "rtweekend.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "texture.hpp"

//unique fog material(dispers the rays in each direction equally)
class isovolumetric : public material {
public:
	isovolumetric(color c) :tex(make_shared<solid_color>(c)) {}
	isovolumetric(shared_ptr<texture> tex) : tex(tex) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		scattered = ray(rec.p, random_unit_vector());
		attenuation = tex->value(rec.u, rec.v, rec.p);
		return true;
	}

private:
	shared_ptr<texture> tex;
};

class constant_medium : public hittable {
public:
	//boundary: fog shape (cube or sphere) d:density, a:color/texture
	constant_medium(shared_ptr<hittable> boundary, double density, shared_ptr<texture> tex)
		: boundary(boundary)
		, neg_inv_density(-1.0 / density)
		, phase_function(make_shared<isovolumetric>(tex))
	{}

	constant_medium(shared_ptr<hittable> boundary, double density, color c)
		: boundary(boundary)
		, neg_inv_density(-1.0 / density)
		, phase_function(make_shared<isovolumetric>(c))
	{}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		hit_record rec1, rec2;

		if (!boundary->hit(r, interval::universe, rec1)) return false;
		if (!boundary->hit(r, interval(rec1.t + 0.0001, infinity), rec2)) return false;

		if (rec1.t < ray_t.min) rec1.t = ray_t.min;
		if (rec2.t > ray_t.max) rec2.t = ray_t.max;

		if (rec1.t >= rec2.t) return false;
		if (rec1.t < 0) rec1.t = 0;

		auto ray_length = r.direction().length();
		auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
		auto hit_distance = neg_inv_density * log(random_double());

		if (hit_distance > distance_inside_boundary) return false;

		rec.t = rec1.t + hit_distance / ray_length;
		rec.p = r.at(rec.t);
		rec.normal = vec3(1, 0, 0);  //arbitrary, irrelevant when dispersed
		rec.front_face = true;
		rec.mat = phase_function;

		return true;
	}

	aabb bounding_box() const override {
		return boundary->bounding_box();
	}

private:
	shared_ptr<hittable> boundary;
	double neg_inv_density;
	shared_ptr<material> phase_function;
};
