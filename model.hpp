#pragma once

#include "tiny_obj_loader.h"
#include "hittable_list.hpp"
#include "bvh.hpp"
#include "triangle.hpp"

#include <iostream>

class model : public hittable {
public:
	model(const std::string& filename, shared_ptr<material> mat, double scale = 1.0) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
			std::cerr << "Can not load the model: " << warn << err << std::endl;
			return;
		}

		//calculate center of the model  
		double min_x = infinity, min_y = infinity, min_z = infinity;
		double max_x = -infinity, max_y = -infinity, max_z = -infinity;

		for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
			min_x = fmin(min_x, attrib.vertices[i + 0]);
			min_y = fmin(min_y, attrib.vertices[i + 1]);
			min_z = fmin(min_z, attrib.vertices[i + 2]);
			max_x = fmax(max_x, attrib.vertices[i + 0]);
			max_y = fmax(max_y, attrib.vertices[i + 1]);
			max_z = fmax(max_z, attrib.vertices[i + 2]);
		}

		//calculate offset to move center of the model to (0,0,0) 
		//and set its bottom (min_y) to y=0
		vec3 center_offset(
			(min_x + max_x) / 2.0,
			min_y, //subtract min_y to set bottom to y=0
			(min_z + max_z) / 2.0
		);

		hittable_list triangles;

		//create triangles from loaded model using offset and scale
		for (const auto& shape : shapes) {
			size_t index_offset = 0;
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
				// Assume the model is made of triangles
				tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
				tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
				tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

				auto get_v = [&](tinyobj::index_t idx) {
					return point3(
						(attrib.vertices[3 * idx.vertex_index + 0] - center_offset.x()) * scale,
						(attrib.vertices[3 * idx.vertex_index + 1] - center_offset.y()) * scale,
						(attrib.vertices[3 * idx.vertex_index + 2] - center_offset.z()) * scale
					);
				};

				triangles.add(make_shared<triangle>(get_v(idx0), get_v(idx1), get_v(idx2), mat));
				index_offset += 3;
			}
		}
		//BVH only for this model
		mesh_bvh = make_shared<bvh_node>(triangles);

		//debug infos
		std::cout << "Model: " << filename << " loaded (" << triangles.objects.size() << " triangles)." << std::endl;

		aabb final_box = mesh_bvh->bounding_box();
		std::cout << "Model: " << filename << " centered at (0,0,0)\n";
		std::cout << "New Min: " << final_box.x.min << ", " << final_box.y.min << "\n";
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		return mesh_bvh->hit(r, ray_t, rec);
	}

	aabb bounding_box() const override {
		return mesh_bvh->bounding_box();
	}

private:
	shared_ptr<bvh_node> mesh_bvh;
};
