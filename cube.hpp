#pragma once	

#include "hittable.hpp"
#include "vec3.hpp"

//class cube
class cube : public hittable {
public:
	cube(const point3& min_corner, const point3& max_corner, shared_ptr<material> mat)
		: min_corner(min_corner)
		, max_corner(max_corner)
		, mat(mat)
	{}

	//cube
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		//loop "for" goes through each axis checking interesection
		for (int i = 0; i < 3; ++i) {
			//distances of intersections of radius with surfaces limting cube 
			double t0 = (min_corner[i] - r.origin()[i]) / r.direction()[i];
			double t1 = (max_corner[i] - r.origin()[i]) / r.direction()[i];

			//if t0 
			if (t0 > t1) {
				std::swap(t0, t1);
			}
			ray_t.min = std::fmax(t0, ray_t.min);
			ray_t.max = std::fmin(t1, ray_t.max);

			//no interesection
			if (ray_t.max <= ray_t.min) {
				return false;
			}
		}

		//update hit_record data if radius intersect the cube
		rec.t = ray_t.min; //set distance from starting point of the radius to the place of intersection
		rec.p = r.at(ray_t.min); //calculate interection point using r.at(t_min) function
		rec.normal = compute_normal(rec.p); //determine normal
		rec.mat = mat; //assign cube material
		return true;
	}

private:
	point3 min_corner, max_corner; //bottom-left-back and top-rigt-front cube corners
	shared_ptr<material> mat;

	//function to determine normal vector in intersection
	vec3 compute_normal(const point3& p) const {
		//left cube surface 
		if (std::fabs(p.x() - min_corner.x()) < 1e-8) {
			return vec3(-1, 0, 0);
		}
		//right cube surface 
		if (std::fabs(p.x() - min_corner.x()) < 1e-8) {
			return vec3(1, 0, 0);
		}
		//bottom cube surface 
		if (std::fabs(p.y() - min_corner.y()) < 1e-8) {
			return vec3(0, -1, 0);
		}
		//top cube surface 
		if (std::fabs(p.y() - min_corner.y()) < 1e-8) {
			return vec3(0, 1, 0);
		}
		//front cube surface 
		if (std::fabs(p.z() - min_corner.z()) < 1e-8) {
			return vec3(0, 0, -1);
		}
		//back cube surface
		return vec3(0, 0, 1);
	}
};
