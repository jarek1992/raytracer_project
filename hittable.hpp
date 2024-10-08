#pragma once

#include "ray.hpp"

class hit_record {
public:
    point3 p;
    vec3 normal;
    double t;
};
//virtual abstract class 
class hittable {
public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const = 0;
};