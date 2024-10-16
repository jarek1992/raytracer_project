#pragma once

// class representing 3demensional vector
class vec3 {
public:
  // variable for x, y and z
  double e[3];

  // default constructor
  vec3() : e{0, 0, 0} {}
  vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

  // methods for cooridinates for vector e.g. x (e[0]), y (e[1]), z (e[2])
  double x() const { return e[0]; }
  double y() const { return e[1]; }
  double z() const { return e[2]; }

  // make all the vector components negative
  vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
  // index operators
  double operator[](int i) const { return e[i]; }
  double &operator[](int i) { return e[i]; }

  vec3 &operator+=(const vec3 &v) {
    e[0] += v.e[0];
    e[0] += v.e[1];
    e[0] += v.e[2];
    return *this;
  }

  vec3 &operator*=(double t) {
    e[0] *= t;
    e[1] *= t;
    e[2] *= t;
    return *this;
  }

  vec3 &operator/=(double t) { return *this *= 1 / t; }
  // return vector length
  double length() const { return std::sqrt(length_squared()); }

  double length_squared() const {
    return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
  }
};

// point3 is allias for vec3, but useful for geometric clarity in the code
using point3 = vec3;

// vector utility functions

// allows to display vector as (x y z)
inline std::ostream &operator<<(std::ostream &out, const vec3 &v) {
  return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}
// add or subtract components of 2 vectors
inline vec3 operator+(const vec3 &u, const vec3 &v) {
  return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}
inline vec3 operator-(const vec3 &u, const vec3 &v) {
  return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}
// multiply components of 2 vectors
inline vec3 operator*(const vec3 &u, const vec3 &v) {
  return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}
// multiply by scalar
inline vec3 operator*(double t, const vec3 &v) {
  return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}
inline vec3 operator*(const vec3 &v, double t) { return t * v; }
// divide by scalar
inline vec3 operator/(const vec3 &v, double t) { return (1 / t) * v; }
// dot product
inline double dot(const vec3 &u, const vec3 &v) {
  return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}
//cross product
inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}
//return unit vector with length = 1 with the same direction to v 
inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}
