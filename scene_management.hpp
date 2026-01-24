#pragma once

//basic types and material library
#include "rtweekend.hpp"
#include "material_library.hpp"
//geometry (shapes)
#include "hittable_list.hpp"
#include "sphere.hpp"
#include "cube.hpp"
#include "triangle.hpp"
#include "model.hpp"
#include "constant_medium.hpp" //fog
//transformation and instances
#include "material_instance.hpp"
#include "translate.hpp"
#include "rotate_x.hpp"
#include "rotate_y.hpp"
#include "rotate_z.hpp"
#include "scale.hpp"

//headers
#include <memory>
#include <vector>
#include <string>

//loading materials
void load_materials(MaterialLibrary& mat_lib) {
	//bump map textures
	auto wood_bump = make_shared<image_texture>("assets/bump_maps/wood_bump_map.jpg");
	auto scratches_bump = make_shared<image_texture>("assets/bump_maps/scratches_bump_map.jpg");
	auto concrete_bump = make_shared<image_texture>("assets/bump_maps/concrete_bump_map.jpg");
	auto water_bump = make_shared<image_texture>("assets/bump_maps/water_bump_map.jpg");

	//add some predefined materials to the library
	mat_lib.add("water", make_shared<dielectric>(1.33, water_bump, 0.8));
	mat_lib.add("turquoise_water", make_shared<dielectric>(1.33, color(0.85, 1.0, 0.98), water_bump, 2.0));

	mat_lib.add("red_diffuse", make_shared<lambertian>(color(0.8, 0.1, 0.1)));
	mat_lib.add("rough_gold", make_shared<metal>(color(1.0, 0.84, 0.0), 0.15));
	mat_lib.add("light_blue_diffuse", make_shared<lambertian>(color(0.1, 0.4, 0.9)));
	mat_lib.add("white_diffuse", make_shared<lambertian>(color(0.9, 0.9, 0.9)));

	mat_lib.add("wood_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg")));
	mat_lib.add("wood_bumpy_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"), wood_bump, 2.0));

	mat_lib.add("gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0));
	mat_lib.add("scratched_gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0, scratches_bump, -2.0));

	mat_lib.add("mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("scratched_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0, scratches_bump, 1.0));

	mat_lib.add("brushed_aluminium", make_shared<metal>(color(1.0, 1.0, 1.0), 0.25));
	mat_lib.add("metal_colored", make_shared<metal>(color(0.2, 0.8, 0.2), 0.05));
	mat_lib.add("checker_texture", make_shared<lambertian>(make_shared<checker_texture>(0.5, color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9))));
	mat_lib.add("glass_bubble", make_shared<dielectric>(1.0 / 1.5));

	mat_lib.add("glass", make_shared<dielectric>(1.5));
	mat_lib.add("foggy_glass", make_shared<dielectric>(1.5, concrete_bump, 0.02));

	mat_lib.add("pure_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("random_diffuse", make_shared<lambertian>(color::random() * color::random()));
	mat_lib.add("random_neon_light", make_shared<diffuse_light>(color::random(0.1, 1.0) * 1.5));
	mat_lib.add("neon_pink", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 6.0));
	mat_lib.add("neon_blue", make_shared<diffuse_light>(color(0.0, 0.5, 1.0) * 6.0));
	mat_lib.add("neon_green", make_shared<diffuse_light>(color(0.1, 1.0, 0.1) * 6.0));
	mat_lib.add("neon_yellow", make_shared<diffuse_light>(color(1.0, 0.8, 0.0) * 6.0));
	mat_lib.add("neon_white", make_shared<diffuse_light>(color(1.0, 1.0, 1.0) * 6.0));
	mat_lib.add("neon_red", make_shared<diffuse_light>(color(1.0, 0.1, 0.1) * 6.0));
	mat_lib.add("ceiling_light", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 10.0));
	//... add more materials as needed

	//floor checker material 
	auto checker = make_shared<checker_texture>(0.5, color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
	mat_lib.add("reflective_checker_mat", make_shared<metal>(checker, 0.02));

}

//build scene geometry
hittable_list build_geometry(MaterialLibrary& mat_lib) {
	//global material library
	hittable_list world;

	// - 1. FLOOR -
	auto ground_geom = make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, nullptr);
	world.add(make_shared<material_instance>(ground_geom, mat_lib.get("reflective_checker_mat")));

	// - 2. FREE STANDING GEOMETRIES (in the middle)
	//
	//teapot (loaded object .obj from the file)
	auto teapot_base = make_shared<model>("assets/models/teapot.obj", nullptr, 0.4);
	auto teapot_inst = make_shared<material_instance>(teapot_base, mat_lib.get("glass"));
	auto rot_teapot_x = make_shared<rotate_x>(teapot_inst, -90.0);
	auto rot_teapot_y = make_shared<rotate_y>(rot_teapot_x, 30.0);
	auto teapot_final = make_shared<translate>(rot_teapot_y, point3(0.0, 1.0, -2.5));
	world.add(teapot_final);
	//sphere
	auto big_sphere_geom = make_shared<sphere>(point3(0.0, 0.0, 0.0), 1.0, nullptr);
	auto big_sphere_instance = make_shared<material_instance>(big_sphere_geom, mat_lib.get("scratched_mirror"));
	world.add(make_shared<translate>(big_sphere_instance, point3(0.0, 1.0, 0.0)));
	//small sphere #1
	auto small_sphere_geom = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.5, nullptr);
	auto small_sphere_instance = make_shared<material_instance>(small_sphere_geom, mat_lib.get("scratched_gold_mat"));
	world.add(make_shared<translate>(small_sphere_instance, point3(3.0, 0.5, -1.0)));
	//small sphere #2
	auto small_bubble_instance = make_shared<material_instance>(small_sphere_geom, mat_lib.get("wood_bumpy_texture"));
	world.add(make_shared<translate>(small_bubble_instance, point3(3.0, 0.5, 1.0)));
	//cube
	auto big_cube_geom = make_shared<cube>(point3(0.0, 0.0, 0.0), nullptr);
	auto big_cube_instance = make_shared<material_instance>(big_cube_geom, mat_lib.get("foggy_glass"));
	world.add(make_shared<translate>(big_cube_instance, point3(0.0, 1.0, 2.5)));

	// - 3. RANDOM SPREADED GEOMETRIES
	//
	//master geometries (local prefabicats)
	auto master_cube = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	auto master_sphere = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.2, nullptr);
	//material filters
	auto neon_mats = mat_lib.get_emissive_names();
	auto regular_mats = mat_lib.get_regular_names();
	//for loop to randomize location of small cubes and spheres*/
	for (int a = -15; a < 15; a++) {
		for (int b = -15; b < 15; b++) { //create the grid a(-15, 15) / b(-15, 15)
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()); //point3(x,(y = const),z)

			if ((center - point3(4.0, 0.2, 0.0)).length() > 0.9) {
				std::string selected_mat_name;
				shared_ptr<hittable> geometry;
				//scale
				vec3 scale_v(1.0, 1.0, 1.0); //default scale
				//select random number 0.0 - 1.0
				double dice_roll = random_double();

				//1. Probability distribution
				if (dice_roll < 0.25 && !neon_mats.empty()) {
					//25% chance to draw neon material
					int max_idx = static_cast<int>(neon_mats.size()) - 1;
					selected_mat_name = neon_mats[random_int(0, max_idx)];
					geometry = master_cube;
					//scale
					scale_v = vec3(0.4, random_double(1.5, 4.5), 0.4);
				}
				else if (dice_roll < 0.55) {
					//30% chance to draw glass material(0.25 + 0.3 = 0.55)
					selected_mat_name = (random_double() < 0.7) ? "glass" : "glass_bubble";
					geometry = master_sphere;
					double s = random_double(0.5, 1.0);
					scale_v = vec3(s, s, s);
				}
				else {
					//45% left draw random material left materials from the list (excluding emissive/neaons using regular_mats)
					int random_idx = random_int(0, static_cast<int>(regular_mats.size()) - 1);
					selected_mat_name = regular_mats[random_idx];

					if (random_double() < 0.5) {
						geometry = master_sphere;
					}
					else {
						geometry = master_cube;
					}
					//scale to difference
					double s = random_double(0.8, 1.2);
					scale_v = vec3(s, s, s);

				}
				//2. get material
				auto obj_mat = mat_lib.get(selected_mat_name);

				//3. scale
				auto scaled_obj = make_shared<scale>(geometry, scale_v);
				//4. rotation (only cubes)
				shared_ptr<hittable> rotated_obj = scaled_obj;
				if (geometry == master_cube) {
					rotated_obj = make_shared<rotate_y>(scaled_obj, random_double(0.0, 90.0));
				}

				//5. applying material to the instance through material_instance class
				auto instance = make_shared<material_instance>(rotated_obj, obj_mat);
				//3. final position translation
				world.add(make_shared<translate>(instance, center));
			}
		}
	}

	// - 4. LIGHTS 
	auto light_geom = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	auto light_instance = make_shared<material_instance>(light_geom, mat_lib.get("ceiling_light"));
	world.add(make_shared<translate>(light_instance, point3(0.0, 15.0, 0.0)));

	// - 5. ENVIRONMENTAL FOG
	auto fog_boundary = make_shared<sphere>(point3(0.0, 0.0, 0.0), 30.0, nullptr);
	world.add(make_shared<constant_medium>(fog_boundary, 0.1, color(0.0, 0.5, 1.0))); //fog density: 0.01 - delicate fog, 0.1 - dense fog

	return world;
}