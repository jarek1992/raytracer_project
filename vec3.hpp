#pragma once

#include <cmath>
#include <iostream>

//class representing 3-dimensional vector
class vec3 {
public:
	//variable for x, y and z
	double e[3];

	//default constructor: initializes to (0, 0, 0)
	constexpr vec3() : e{ 0, 0, 0 } {}

	//parameterized constructor: initializes to specific values (e0, e1, e2)
	constexpr vec3(double e0, double e1, double e2) : e{ e0, e1, e2 } {}

	//getter methods for individual components (x, y, z))
	constexpr double x() const { 
		return e[0];
	}
	constexpr double y() const { 
		return e[1]; 
	}
	constexpr double z() const {
		return e[2];
	}

	//Unary minus operator to negate the vector
	constexpr vec3 operator-() const { 
		return vec3(-e[0], -e[1], -e[2]); 
	}

	//array index operators
	constexpr double operator[](int i) const { 
		return e[i];
	}
	double& operator[](int i) {
		return e[i];
	}

	//addition assignment (v += u)
	vec3& operator+=(const vec3& v) {
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}
	//multiplication assignment (v *= scalar)
	vec3& operator*=(double t) {
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator*=(const vec3& v) {
		e[0] *= v.e[0];
		e[1] *= v.e[1];
		e[2] *= v.e[2];
		return *this;
	}

	//division assignment (v /= scalar)
	vec3& operator/=(double t) {
		return *this *= 1 / t;
	}

	//length (magnitude) of the vector
	double length() const {
		return std::sqrt(length_squared());
	}

	//length squared (no square root for efficiency)
	constexpr double length_squared() const {
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}

	bool near_zero() const {
		//return true if the vector is close to zero in all dimensions.
		auto s = 1e-8;
		return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
	}

	//static methods to generate random vectors
	static vec3 random() {
		return vec3(random_double(), random_double(), random_double());
	}
	static vec3 random(double min, double max) {
		return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
	}

	//luminance calculation using Rec. 709 coefficients
	double luminance() const {
		return 0.2126 * e[0] + 0.7152 * e[1] + 0.0722 * e[2];
	}
};

//point3 is allias for vec3, but useful for geometric clarity in the code
using point3 = vec3;
//color is allias for vec3, but useful for clarity in the code
using color = vec3;

//vector utility functions
//
//allows to display vector as (x y z)
inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
	return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}

//add or subtract components of 2 vectors
inline vec3 operator+(const vec3& u, const vec3& v) {
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

//vector subtraction (u - v)
inline vec3 operator-(const vec3& u, const vec3& v) {
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

//multiply components of 2 vectors
inline vec3 operator*(const vec3& u, const vec3& v) {
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

//multiply by scalar
inline vec3 operator*(double t, const vec3& v) {
	return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

//scalar multiplication (v * t)
inline vec3 operator*(const vec3& v, double t) {
	return t * v;
}

//scalar division (v / t)
inline vec3 operator/(const vec3& v, double t) {
	return (1 / t) * v;
}

//dot product of two vectors (u . v)
inline double dot(const vec3& u, const vec3& v) {
	return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}
// cross product
inline vec3 cross(const vec3& u, const vec3& v) {
	return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
		u.e[2] * v.e[0] - u.e[0] * v.e[2],
		u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

//return unit vector with length = 1 with the same direction to v
inline vec3 unit_vector(const vec3& v) {
	double len = v.length();
	if (len < 1e-8) return vec3(0, 0, 0); // Zabezpieczenie przed zerem
	return v / len;
}

//generate random vector inside unit disk on XY surface (disk with radius 1 and center(0,0,0))
inline vec3 random_in_unit_disk() {
	while (true) {
		auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0); //generate random point in 2D surface
		if (p.length_squared() < 1) { //checking if point is located on disk
			return p; //return random point located inside determined disk
		}
	}
}

//generate a random unit vecto
inline vec3 random_unit_vector() {
	while (true) {
		auto p = vec3::random(-1, 1);
		auto lensq = p.length_squared();
		if (1e-160 < lensq && lensq <= 1)
			return p / sqrt(lensq);
	}
}

//generate a random vector on the hemisphere defined by the normal
inline vec3 random_on_hemisphere(const vec3& normal) {
	vec3 on_unit_sphere = random_unit_vector();
	if (dot(on_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
		return on_unit_sphere;
	else
		return -on_unit_sphere;
}

//vec3 reflection function
inline vec3 reflect(const vec3& v, const vec3& n) {
	return v - 2 * dot(v, n) * n;
}

//refraction function (Snell's Law)
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
	auto cos_theta = std::fmin(dot(-uv, n), 1.0); //angle of incidence (cos⁡θ1)
	vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n); //perpendicular component (r_out_perp​)
	vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n; // parallel component (r_out_parallel​)
	return r_out_perp + r_out_parallel; // refraction vector
}