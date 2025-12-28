#pragma once

#include "hittable_list.hpp"
#include "tiny_obj_loader.hpp"
#include "bvh.hpp"
#include "triangle.hpp"

#include <iostring>

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

		hittable_list triangles;

		for (const auto& shape : shapes) {
			size_t index_offset = 0;
			for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
				// Assume the model is made of triangles
				tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
				tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
				tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

				point3 v0(attrib.vertices[3 * idx0.vertex_index + 0],
					attrib.vertices[3 * idx0.vertex_index + 1],
					attrib.vertices[3 * idx0.vertex_index + 2]);
				point3 v1(attrib.vertices[3 * idx1.vertex_index + 0],
					attrib.vertices[3 * idx1.vertex_index + 1],
					attrib.vertices[3 * idx1.vertex_index + 2]);
				point3 v2(attrib.vertices[3 * idx2.vertex_index + 0],
					attrib.vertices[3 * idx2.vertex_index + 1],
					attrib.vertices[3 * idx2.vertex_index + 2]);

				triangles.add(make_shared<triangle>(v0 * scale, v1 * scale, v2 * scale, mat));
				index_offset += 3;
			}
		}
		//BVH only for this model
		mesh_bvh = make_shared<bvh_node>(triangles);
		std::cout << "Model: " << filename << " loaded (" << triangles.objects.size() << " triangles)." << std::endl;
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
