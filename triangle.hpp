#pragma once

#include "hittable.hpp"

class triangle : public hittable {
public:
	triangle(const point3& a, const point3& b, const point3& c, const vec3& _n0, const vec3& _n1, const vec3& _n2, shared_ptr<material>m)
		: v0(a)
		, v1(b)
		, v2(c)
		, n0(_n0)
		, n1(_n1)
		, n2(_n2)
		, mat_ptr(m)
	{}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const {
		//edges
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		//normal 
		vec3 normal = cross(edge1, edge2);
		double normal_length = normal.length();

		if (normal_length < 1e-8) {
			return false; // Degenerate triangle
		}
		vec3 unit_normal = normal / normal_length;

		//check if cut the plane
		double NdotD = dot(unit_normal, r.direction());
		//if the ray is parallel or points away from the triangle
		if (std::abs(NdotD) < 1e-8) {
			return false;
		}

		double D = dot(unit_normal, v0); // D from plane equation N*P = D
		double t = (D - dot(unit_normal, r.origin())) / NdotD;

		if (!ray_t.contains(t)) {
			return false;
		}
		//find the intersection point and check if it's inside the triangle
		point3 p = r.at(t);

		//point p tests with respect to edges(outside-in test)
		//checking if P-point is located on the internal vector side for each edge

		//calculate vectors for edges
		vec3 edge_v0v1 = v1 - v0;
		vec3 edge_v1v2 = v2 - v1;
		vec3 edge_v2v0 = v0 - v2;

		//calculate barycentric coordinates for normal interpolation (triangles areas method)
		vec3 C0 = cross(v1 - v0, p - v0);
		vec3 C1 = cross(v2 - v1, p - v1);
		vec3 C2 = cross(v0 - v2, p - v2);

		//inside-outside test
		if (dot(normal, C0) < 0 || dot(normal, C1) < 0 || dot(normal, C2) < 0) {
			return false;
		}

		double area_total_sq = dot(normal, normal); //normal.length()^2
		//u,v,w are the ratios of the small triangle areas to the whole
		double u = dot(normal, C2) / area_total_sq; //weight for vertex v1
		double v = dot(normal, C0) / area_total_sq; //weight for vertex v2
		double w = 1.0 - u - v; //weight for vertex v0

		//interpolated normal
		vec3 smooth_normal = unit_vector(w * n0 + u * n1 + v * n2);

		//if we reach this point, the ray hits the triangle
		rec.t = t;
		rec.p = p;
		rec.mat = mat_ptr;
		rec.set_face_normal(r, smooth_normal);

		return true;
	}

	aabb bounding_box() const override {
		//searching for min and max points for each axis
		double min_x = fmin(v0.x(), fmin(v1.x(), v2.x()));
		double min_y = fmin(v0.y(), fmin(v1.y(), v2.y()));
		double min_z = fmin(v0.z(), fmin(v1.z(), v2.z()));

		double max_x = fmax(v0.x(), fmax(v1.x(), v2.x()));
		double max_y = fmax(v0.y(), fmax(v1.y(), v2.y()));
		double max_z = fmax(v0.z(), fmax(v1.z(), v2.z()));

		//adding small margin (padding) to avoid zero thickness
		double delta = 0.0001;
		if (max_x - min_x < delta) { min_x -= delta; max_x += delta; }
		if (max_y - min_y < delta) { min_y -= delta; max_y += delta; }
		if (max_z - min_z < delta) { min_z -= delta; max_z += delta; }

		return aabb(point3(min_x, min_y, min_z), point3(max_x, max_y, max_z));
	}

	void set_material(std::shared_ptr<material> m) {
		mat_ptr = m;
	}

private:
	point3 v0, v1, v2;
	vec3 n0, n1, n2; //new areas
	shared_ptr<material> mat_ptr;
};