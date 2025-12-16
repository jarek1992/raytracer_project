#pragma once	

#include "hittable.hpp"
#include "vec3.hpp"

//class cube
class cube : public hittable {
public:
	//constructor using min and max corners in world space (3 args)
	//converts boundaries to half extents
	cube(const point3& min_corner_world, const point3& max_corner_world,
		shared_ptr<material> mat)
		: mat(mat)
	{
		//calculate half extents from min and max corners
		half_extents = 0.5 * (max_corner_world - min_corner_world);
	}

	//constructor using center position and fixed size (2 args)
	//uses to define cube with constant size centered at given position(e.g., side length 2.0) and half extents = 1,1,1)
	cube(const point3& center_pos, shared_ptr<material> mat)
		: mat(mat)
	{
		//set constant size (e.g., half-size 1.0 in each axis)
		half_extents = vec3(1.0, 1.0, 1.0);
	}

	//cube
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		double tmin = ray_t.min;
		double tmax = ray_t.max;

		//loop "for" goes through each axis checking interesection
		for (int i = 0; i < 3; ++i) {
			//boundaries of cube in local space (centered at origin (0,0,0))
			double min_local = -half_extents[i];
			double max_local = half_extents[i];
			//distances of intersections of radius with surfaces limting cube 
			double invD = 1.0 / r.direction()[i];
			double t0 = (min_local - r.origin()[i]) * invD;
			double t1 = (max_local - r.origin()[i]) * invD;

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

		//local intersection point and normal calculation
		rec.t = tmin; //set distance from starting point of the radius to the place of intersection
		rec.p = r.at(rec.t);

		//updated compute normal function
		rec.normal = compute_normal(rec.p);

		rec.mat = mat; //assign cube material
		rec.set_face_normal(r, rec.normal);
	
		return true;
	}

private:
	vec3 half_extents; //half the size of the cube in each dimension
	shared_ptr<material> mat;

	//function to determine normal vector in intersection
	vec3 compute_normal(const point3& p) const {
		const double EPS = 1e-3;

		if (std::fabs(p.x() + half_extents.x()) < EPS) return vec3(-1, 0, 0); //mix_x (back face)
		if (std::fabs(p.x() - half_extents.x()) < EPS) return vec3(1, 0, 0); //max_x (front face)

		if (std::fabs(p.y() + half_extents.y()) < EPS) return vec3(0, -1, 0); //min_y (bottom face)
		if (std::fabs(p.y() - half_extents.y()) < EPS) return vec3(0, 1, 0); //max_y (top face)

		if (std::fabs(p.z() + half_extents.z()) < EPS) return vec3(0, 0, -1); //min_z (left face)
		return vec3(0, 0, 1);
	}
};
