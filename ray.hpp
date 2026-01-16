#pragma once

#include "vec3.hpp"

class ray {
public:
	point3 orig;
	vec3 dir;
	double tm = 0.0;

	//default constructor
	ray() {}

	//2 args 
	ray(const point3& origin, const vec3& direction)
		: orig(origin), dir(direction), tm(0.0) // tm set to 0.0
	{}

	// 3 args parametric constructor, creates the object with initial values
	ray(const point3& origin, const vec3& direction, double time)
		: orig(origin)
		, dir(direction)
		, tm(time)
	{}

	//getters
	const point3& origin() const {
		return orig;
	}
	const vec3& direction() const {
		return dir;
	}
	double time() const {
		return tm;
	}

	//return the point at radius for parameter t
	point3 at(double t) const {
		return orig + t * dir;
	}
};