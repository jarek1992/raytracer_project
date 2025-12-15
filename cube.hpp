#pragma once	

#include "hittable.hpp"
#include "vec3.hpp"

//class cube
class cube : public hittable {
public:
	cube(const point3& min_corner_world, const point3& max_corner_world, 
		shared_ptr<material> mat, double angle)
		: mat(mat)
		, rotation_Y(angle)
	{
		//calculate center position of cube in world coordinates
		position = 0.5 * (min_corner_world + max_corner_world);
		//calculate half extents vector (half the size of the cube in each dimension)
		half_extents = 0.5 * (max_corner_world - min_corner_world);
	}

	//cube
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		
		//WORLD -> LOCAL (substract center and rotate)
		//cube center point
		point3 C = position;
		vec3 ro = r.origin() - C;
		vec3 rd = r.direction();

		ro = rotate_Y(ro, -rotation_Y);
		rd = rotate_Y(rd, -rotation_Y);

		ray r_local(ro, rd);

		double tmin = ray_t.min;
		double tmax = ray_t.max;

		//loop "for" goes through each axis checking interesection
		for (int i = 0; i < 3; ++i) {
			//boundaries of cube in local space (centered at origin (0,0,0))
			double min_local = -half_extents[i];
			double max_local = half_extents[i];

			//calculate intersection 
			
			//distances of intersections of radius with surfaces limting cube 
			double invD = 1.0 / r_local.direction()[i];
			double half_extent = half_extents[i];

			double t0 = (min_local - r_local.origin()[i]) * invD;
			double t1 = (max_local - r_local.origin()[i]) * invD;

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
		point3 p_local = r_local.at(rec.t);

		//updated compute normal function
		vec3 normal_local = compute_normal(p_local);

		//local->World (rotate and add center)
		rec.p = rotate_Y(p_local, rotation_Y) + C;
		vec3 normal_world = rotate_Y(normal_local, rotation_Y);
		rec.normal = unit_vector(normal_world); //determine normal
		
		rec.mat = mat; //assign cube material
		rec.set_face_normal(r, rec.normal);

		return true;
	}

private:
	point3 position; //cube center position
	vec3 half_extents; //half the size of the cube in each dimension
	shared_ptr<material> mat;
	double rotation_Y;

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

	//functions to rotate cubes Z-axis
	vec3 rotate_Y(const vec3& v, double angle) const {
		double c = std::cos(angle);
		double s = std::sin(angle);
		return vec3(
			c * v.x() + s * v.z(),  // x' = c*x + s*z
			v.y(),                  // y' = y
			-s * v.x() + c * v.z()  // z' = -s*x + c*z
		);
	}
};
