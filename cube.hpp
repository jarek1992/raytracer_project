#pragma once	

#include "hittable.hpp"
#include "vec3.hpp"

//class cube
class cube : public hittable {
public:
	cube(const point3& min_corner, const point3& max_corner, shared_ptr<material> mat, double angle)
		: min_corner(min_corner)
		, max_corner(max_corner)
		, mat(mat)
		, rotation_Z(angle)
	{}

	//cube
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

		//rotate the radius locally
		vec3 ro = rotate_Z_inv(r.origin(), rotation_Z);
		vec3 rd = rotate_Z_inv(r.direction(), rotation_Z);
		ray r_local(ro, rd);

		double tmin = ray_t.min;
		double tmax = ray_t.max;

		//loop "for" goes through each axis checking interesection
		for (int i = 0; i < 3; ++i) {
			//distances of intersections of radius with surfaces limting cube 
			double invD = 1.0 / r_local.direction()[i];
			double t0 = (min_corner[i] - r_local.origin()[i]) * invD;
			double t1 = (max_corner[i] - r_local.origin()[i]) * invD;

			if (invD < 0.0) {
				std::swap(t0, t1);
			}
			tmin = std::fmax(t0, tmin);
			tmax = std::fmin(t1, tmax);

			//no interesection
			if (tmax < tmin) {
				return false;
			}
		}

		//update hit_record data if radius intersect the cube
		rec.t = tmin; //set distance from starting point of the radius to the place of intersection
		point3 p_local = r_local.at(rec.t);
		vec3 normal_local = compute_normal(p_local);
		rec.p = rotate_Z(p_local, rotation_Z); //calculate interection point using r.at(tmin) function
		rec.normal = unit_vector(rotate_Z(normal_local, rotation_Z)); //determine normal
		rec.mat = mat; //assign cube material
		rec.set_face_normal(r, rec.normal);

		return true;
	}

private:
	point3 min_corner, max_corner; //bottom-left-back and top-rigt-front cube corners
	shared_ptr<material> mat;
	double rotation_Z;

	//function to determine normal vector in intersection
	vec3 compute_normal(const point3& p) const {
		const double EPS = 1e-3;

		if (std::fabs(p.x() - min_corner.x()) < EPS) return vec3(-1, 0, 0);
		if (std::fabs(p.x() - max_corner.x()) < EPS) return vec3(1, 0, 0);

		if (std::fabs(p.y() - min_corner.y()) < EPS) return vec3(0, -1, 0);
		if (std::fabs(p.y() - max_corner.y()) < EPS) return vec3(0, 1, 0);

		if (std::fabs(p.z() - min_corner.z()) < EPS) return vec3(0, 0, -1);
		return vec3(0, 0, 1);
	}

	//functions to rotate cubes Z-axis
	vec3 rotate_Z(const vec3& v, double angle) const {
		double c = std::cos(angle);
		double s = std::sin(angle);
		return vec3(
			c * v.x() - s * v.y(),
			s * v.x() - c * v.y(),
			v.z()
		);
	}

	vec3 rotate_Z_inv(const vec3& v, double angle) const {
		double c = std::cos(angle);
		double s = std::sin(angle);
		return vec3(
			c * v.x() + s * v.y(),
			-s * v.x() + c * v.y(),
			v.z()
		);
	}
};
