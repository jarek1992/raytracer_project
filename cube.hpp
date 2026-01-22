#pragma once	

#include "hittable.hpp"
#include "material.hpp"

//class cube
class cube : public hittable {
public:
	//1. constructor using min and max corners in world space (3 args)
	//converts boundaries to half extents
	cube(const point3& min_corner_world, const point3& max_corner_world,
		shared_ptr<material> mat)
		: mat(mat)
		, min_p(min_corner_world)
		, max_p(max_corner_world)
	{
		//calculate half extents from min and max corners
		half_extents = 0.5 * (max_corner_world - min_corner_world);
		center = min_corner_world + half_extents; //calculate center position
	}

	//2. constructor using center position and fixed size (2 args)
	//uses to define cube with constant size centered at given position(e.g., side length 2.0) and half extents = 1,1,1)
	cube(const point3& center_pos, shared_ptr<material> mat)
		: mat(mat)
		, center(center_pos)
	{
		//set constant size (e.g., half-size 1.0 in each axis)
		half_extents = vec3(1.0, 1.0, 1.0);
		min_p = center_pos - half_extents; //calculate min corner
		max_p = center_pos + half_extents; //calculate max corner
	}

	aabb bounding_box() const override {
		double delta = 0.0001;
		return aabb(
			interval(min_p.x(), max_p.x()).expand(delta),
			interval(min_p.y(), max_p.y()).expand(delta),
			interval(min_p.z(), max_p.z()).expand(delta)
		);
	}

	//cube
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		point3 relative_origin = r.origin() - center; //translate ray origin to cube local space

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

		//calculate local vector in cube space (centered at origin)
		vec3 local_p = rec.p - center;

		//set normal,UV and tangents
		set_cube_hit_data(local_p, rec);

		rec.mat = mat; //assign cube material
		rec.set_face_normal(r, rec.normal);

		return true;
	}

	void set_material(std::shared_ptr<material> m) {
		mat = m;
	}

private:
	vec3 half_extents; //half the size of the cube in each dimension
	point3 center; //center position of the cube
	shared_ptr<material> mat;
	point3 min_p;
	point3 max_p;

	////function to determine normal vector in intersection
	//vec3 compute_normal(const point3& p) const {
	//	const double EPS = 1e-3;

	//	if (std::fabs(p.x() + half_extents.x()) < EPS) return vec3(-1, 0, 0); //mix_x (back face)
	//	if (std::fabs(p.x() - half_extents.x()) < EPS) return vec3(1, 0, 0); //max_x (front face)

	//	if (std::fabs(p.y() + half_extents.y()) < EPS) return vec3(0, -1, 0); //min_y (bottom face)
	//	if (std::fabs(p.y() - half_extents.y()) < EPS) return vec3(0, 1, 0); //max_y (top face)

	//	if (std::fabs(p.z() + half_extents.z()) < EPS) return vec3(0, 0, -1); //min_z (left face)
	//	return vec3(0, 0, 1);
	//}

	//function to set hit record data (normal, UV coordinates, tangent, bitangent)
	void set_cube_hit_data(const vec3& p, hit_record& rec) const {
		const double EPS = 1e-3;

		//side X (left/right)
		if (std::fabs(p.x() + half_extents.x()) < EPS) { // MIN_X
			rec.normal = vec3(-1, 0, 0);
			rec.u = (p.z() + half_extents.z()) / (2 * half_extents.z());
			rec.v = (p.y() + half_extents.y()) / (2 * half_extents.y());
			rec.tangent = vec3(0, 0, 1);
		} else if (std::fabs(p.x() - half_extents.x()) < EPS) { // MAX_X
			rec.normal = vec3(1, 0, 0);
			rec.u = (p.z() + half_extents.z()) / (2 * half_extents.z());
			rec.v = (p.y() + half_extents.y()) / (2 * half_extents.y());
			rec.tangent = vec3(0, 0, -1);
		}
		//side Y (bottom/top)
		else if (std::fabs(p.y() + half_extents.y()) < EPS) { // MIN_Y
			rec.normal = vec3(0, -1, 0);
			rec.u = (p.x() + half_extents.x()) / (2 * half_extents.x());
			rec.v = (p.z() + half_extents.z()) / (2 * half_extents.z());
			rec.tangent = vec3(1, 0, 0);
		} else if (std::fabs(p.y() - half_extents.y()) < EPS) { // MAX_Y
			rec.normal = vec3(0, 1, 0);
			rec.u = (p.x() + half_extents.x()) / (2 * half_extents.x());
			rec.v = (p.z() + half_extents.z()) / (2 * half_extents.z());
			rec.tangent = vec3(-1, 0, 0);
		}
		//side Z (front/back)
		else if (std::fabs(p.z() + half_extents.z()) < EPS) { // MIN_Z
			rec.normal = vec3(0, 0, -1);
			rec.u = (half_extents.x() - p.x()) / (2 * half_extents.x());
			rec.v = (p.y() + half_extents.y()) / (2 * half_extents.y());
			rec.tangent = vec3(-1, 0, 0);
		} else { // MAX_Z
			rec.normal = vec3(0, 0, 1);
			rec.u = (p.x() + half_extents.x()) / (2 * half_extents.x());
			rec.v = (p.y() + half_extents.y()) / (2 * half_extents.y());
			rec.tangent = vec3(1, 0, 0);
		}

		//bitangent calculation (orthogonal to normal and tangent)
		rec.bitangent = cross(rec.normal, rec.tangent);
	}
};
