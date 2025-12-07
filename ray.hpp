#pragma once

#include "vec3.hpp"

class ray {
public:
	point3 orig;
	vec3 dir;

	//default constructor
	ray() {}

	//parametric constructor, creates the object with initial values
	ray(const point3& origin, const vec3& direction)
		: orig(origin)
		, dir(direction) {}

	//getters
	const point3& origin() const { return orig; }
	const vec3& direction() const { return dir; }

	//return the point at radius for parameter t
	point3 at(double t) const { return orig + t * dir; }
};