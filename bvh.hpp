#pragma once

#include <algorithm>

#include "common.hpp"
#include "hittable.hpp"
#include "hittable_list.hpp"

class bvh_node : public hittable {
public:
	bvh_node(hittable_list list)
		: bvh_node(list.objects, 0, list.objects.size()) {
	}

	bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
		//randomly choose axis (0,1 or 2) to split on
		int axis = random_int(0, 2);

		auto comparator = (axis == 0) ? box_x_compare
			: (axis == 1) ? box_y_compare
			: box_z_compare;

		size_t object_span = end - start;

		if (object_span == 1) {
			left = right = objects[start];
		} else if (object_span == 2) {
			if (comparator(objects[start], objects[start + 1])) {
				left = objects[start];
				right = objects[start + 1];
			} else {
				left = objects[start + 1];
				right = objects[start];
			}
		} else {
			//sort objects and split in half
			std::sort(objects.begin() + start, objects.begin() + end, comparator);

			auto mid = start + object_span / 2;
			left = make_shared<bvh_node>(objects, start, mid);
			right = make_shared<bvh_node>(objects, mid, end);
		}
		bbox = aabb(left->bounding_box(), right->bounding_box());
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec, int depth = 0, bool debug_wire = false) const override {
		//overwrite bool debug_wire = false
		debug_wire = global_settings::bvh_debug_mode;

		//copy interval only for tests AABB and frames 
		interval bbox_t = ray_t;
		if (!bbox.hit(r, bbox_t)) return false;

		if (debug_wire) {
			//entry point calculated basing on bbox_t.min
			point3 p_entry = r.at(bbox_t.min);
			bool is_leaf = (left == nullptr && right == nullptr);

			//increased thickness for better visibility
			float perspective_thickness = global_settings::bvh_thickness * (0.05f + bbox_t.min * 0.1f);
			bool is_current_debug_level = (global_settings::debug_bvh_level == -1) 
											? is_leaf 
											: (depth == global_settings::debug_bvh_level);

			//draw the frames
			if (is_current_debug_level && bbox.is_on_edge(p_entry, perspective_thickness)) {
				rec.t = bbox_t.min - 0.0001f;
				rec.p = p_entry;
				rec.normal = vec3(0, 0, 1);

				float g = std::min(depth * 0.15f, 1.0f);
				float brightness = 4.0f;

				color base_color = color(0.4f, g, (1.0f - g));
				color debug_color = base_color * brightness;
				rec.mat = make_shared<diffuse_light>(debug_color);
				return true;
			}

			//recursion
			if (is_leaf) { 
				return false; 
			}

			bool hit_left = left->hit(r, ray_t, rec, depth + 1, debug_wire);
			if (hit_left) { 
				ray_t.max = rec.t; 
			}
			bool hit_right = right->hit(r, ray_t, rec, depth + 1, debug_wire);

			bool hit_anything = hit_left || hit_right;

			//volumes
			if (hit_anything && is_current_debug_level) {
				float g = std::min(depth * 0.15f, 1.0f);
				color volume_color = color(0.4f, g, (1.0f - g)) * 0.1f;
				rec.mat = make_shared<diffuse_light>(volume_color);
			}

			return hit_anything;
		}

		//standard path without debbuging
		bool hit_left = left->hit(r, ray_t, rec, depth + 1, false);
		if (hit_left) ray_t.max = rec.t;
		bool hit_right = right->hit(r, ray_t, rec, depth + 1, false);
		return hit_left || hit_right;
	}

	aabb bounding_box() const override {
		return bbox;
	}

private:
	shared_ptr<hittable> left;
	shared_ptr<hittable> right;
	aabb bbox;

	//comparison functions for sorting
	static bool box_compare(
		const shared_ptr<hittable> a, 
		const shared_ptr<hittable> b,
		int axis_index) {
		
		const auto a_axis_range = a->bounding_box().axis(axis_index);
		const auto b_axis_range = b->bounding_box().axis(axis_index);
		return a_axis_range.min < b_axis_range.min;
	}
	static bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 0);
	}
	static bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 1);
	}
	static bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
		return box_compare(a, b, 2);
	}
};