#pragma once

#include "aabb.hpp"

class material; //forward declaration to avoid circular dependency
//holds information about the intersection between a ray and an object
class hit_record {
public:
	point3 p;         //intersection point
	vec3 normal;      //normal vector at the intersection point
	shared_ptr<material> mat; //shared_ptr on material
	double t;         //distance along the ray to the intersection point
	bool front_face;  //flag for front/back face hit;
	double u;        //u texture coordinate
	double v;        //v texture coordinate


	//sets the hit record normal vector, 'outward_normal' is assumed to have unit length
	void set_face_normal(const ray& r, const vec3& outward_normal) {
		front_face = dot(r.direction(), outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};
//virtual abstract class for hittable objects
class hittable {
public:
	virtual ~hittable() = default;
	//pure virtual method that must be implemented by derived classes
	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
	//for bounding box computation to return aabb of the object
	virtual aabb bounding_box() const = 0;
};