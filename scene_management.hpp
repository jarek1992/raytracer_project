#pragma once

//basic types and material library
#include "common.hpp"
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

//struct to hold loaded .obj models assets for the scene
struct sceneAssetsLoader {
	shared_ptr<model> teapot;
	shared_ptr<model> torus;
	shared_ptr<model> bowl;
	shared_ptr<model> cylinder;
	shared_ptr<model> pyramid;
	shared_ptr<model> torus_knot;

	sceneAssetsLoader() {
		teapot = make_shared<model>("assets/models/teapot.obj", nullptr, 0.4);
		torus = make_shared<model>("assets/models/torus.obj", nullptr, 0.4);
		bowl = make_shared<model>("assets/models/bowl.obj", nullptr, 0.02);
		cylinder = make_shared<model>("assets/models/cylinder.obj", nullptr, 0.4);
		pyramid = make_shared<model>("assets/models/pyramid.obj", nullptr, 0.4);
		torus_knot = make_shared<model>("assets/models/torus_knot.obj", nullptr, 0.4);
		//add more .obj models here...
	}
};

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
	mat_lib.add("white_diffuse", make_shared<lambertian>(color(1.0, 1.0, 1.0)));
	mat_lib.add("copper", make_shared<metal>(color(0.95, 0.64, 0.54), 0.0));
	mat_lib.add("rough_copper", make_shared<metal>(color(0.89, 0.58, 0.51), 0.2));
	mat_lib.add("rough_gold", make_shared<metal>(color(1.0, 0.84, 0.0), 0.15));
	mat_lib.add("light_blue_diffuse", make_shared<lambertian>(color(0.1, 0.4, 0.9)));
	mat_lib.add("white_diffuse", make_shared<lambertian>(color(0.9, 0.9, 0.9)));
	mat_lib.add("black_diffuse", make_shared<lambertian>(color(0.1, 0.1, 0.1)));
	mat_lib.add("wood_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg")));
	mat_lib.add("wood_bumpy_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"), wood_bump, 8.0));
	mat_lib.add("gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0));
	mat_lib.add("scratched_gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0, scratches_bump, -1.0));
	mat_lib.add("mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("scratched_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0, scratches_bump, 1.0));
	mat_lib.add("brushed_aluminium", make_shared<metal>(color(1.0, 1.0, 1.0), 0.25));
	mat_lib.add("black_diffuse", make_shared<lambertian>(color(0.05, 0.05, 0.05)));
	mat_lib.add("white_metal", make_shared<metal>(color(1.0, 1.0, 1.0), 0.7));
	mat_lib.add("white_metal_bump", make_shared<metal>(color(0.9, 0.9, 0.9), 0.6, concrete_bump, 2.0));
	mat_lib.add("checker_texture", make_shared<lambertian>(make_shared<checker_texture>(0.5, color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9))));
	mat_lib.add("glass_bubble", make_shared<dielectric>(1.0 / 1.5));
	mat_lib.add("glass", make_shared<dielectric>(1.5));
	mat_lib.add("foggy_glass", make_shared<dielectric>(1.5, concrete_bump, 0.02));
	mat_lib.add("pure_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("random_diffuse", make_shared<lambertian>(color::random() * color::random()));
	mat_lib.add("random_neon_light", make_shared<diffuse_light>(color::random(0.1, 1.0) * 1.5));
	mat_lib.add("neon_pink", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 3.0));
	mat_lib.add("neon_blue", make_shared<diffuse_light>(color(0.0, 0.5, 1.0) * 4.0));
	mat_lib.add("neon_green", make_shared<diffuse_light>(color(0.1, 1.0, 0.1) * 4.0));
	mat_lib.add("neon_yellow", make_shared<diffuse_light>(color(1.0, 0.8, 0.0) * 6.0));
	mat_lib.add("neon_white", make_shared<diffuse_light>(color(1.0, 1.0, 1.0) * 6.0));
	mat_lib.add("neon_red", make_shared<diffuse_light>(color(1.0, 0.1, 0.1) * 6.0));
	mat_lib.add("ceiling_emissive", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 5.0));
	//... add more materials as needed

	//floor checker material reflective
	auto checker = make_shared<checker_texture>(0.5, color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
	mat_lib.add("reflective_checker_mat", make_shared<metal>(checker, 0.02));

	//floor checker material
	auto checker1 = make_shared<checker_texture>(0.5, color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
	mat_lib.add("checker_mat", make_shared<metal>(checker1, 0.95));
}

//build scene geometry
hittable_list build_geometry(MaterialLibrary& mat_lib, const sceneAssetsLoader& assets, bool use_fog, double fog_density, color fog_color) {
	//global material library
	hittable_list world;

	// - 1. FLOOR -
	auto ground_geom = make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, nullptr);
	world.add(make_shared<material_instance>(ground_geom, mat_lib.get("reflective_checker_mat")));

	auto big_cube_geom = make_shared<cube>(point3(-4.0, -0.1, -4.0), point3(4.0, 0.1, 4.0), nullptr);
	auto big_cube_instance = make_shared<material_instance>(big_cube_geom, mat_lib.get("black_diffuse"));

	//// SUFIT I PODŁOGA (oś Y)
	//world.add(make_shared<translate>(big_cube_instance, point3(0.0, 5.0, 2.5)));

	//// ŚCIANY BOCZNE (oś X) - obrót Z
	//auto wall_z_rot = make_shared<rotate_z>(big_cube_instance, 90.0);
	//world.add(make_shared<translate>(wall_z_rot, point3(-4.0, 1.0, 2.5)));
	//world.add(make_shared<translate>(wall_z_rot, point3(4.0, 1.0, 2.5)));

	//// ŚCIANY PRZÓD/TYŁ (oś Z) - obrót X
	//auto wall_x_rot = make_shared<rotate_x>(big_cube_instance, 90.0);
	//world.add(make_shared<translate>(wall_x_rot, point3(0.0, 1.0, -1.5)));
	//world.add(make_shared<translate>(wall_x_rot, point3(0.0, 1.0, 6.5)));

	auto torus_knot_instance = make_shared<material_instance>(assets.torus_knot, mat_lib.get("wood_texture"));
	auto torus_knot_scale = make_shared<scale>(torus_knot_instance, vec3(1.3, 1.3, 1.3));
	auto torus_knot_final = make_shared<translate>(torus_knot_scale, point3(1.0, 0.0, 2.0));
	world.add(torus_knot_final);

	auto teapot_inst = make_shared<material_instance>(assets.teapot, mat_lib.get("glass"));
	auto rot_teapot_x = make_shared<rotate_x>(teapot_inst, -90.0);
	auto rot_teapot_y = make_shared<rotate_y>(rot_teapot_x, 30.0);
	auto teapot_final = make_shared<translate>(rot_teapot_y, point3(0.0, 1.0, -2.0));
	world.add(teapot_final);

	//// - 4. LIGHT PLANE 
	//auto light_geom = make_shared<cube>(point3(-0.5, -0.05, -0.5), point3(0.5, 0.05, 0.5), nullptr);

	//auto light_instance = make_shared<material_instance>(light_geom, mat_lib.get("neon_pink"));
	//world.add(make_shared<translate>(light_instance, point3(0.0, 3.0, 2.5)));

	//auto light_instance1 = make_shared<material_instance>(light_geom, mat_lib.get("neon_green"));
	//auto light_instance1_z_rot = make_shared<rotate_z>(light_instance1, 60.0);
	//world.add(make_shared<translate>(light_instance1_z_rot, point3(-2.0, 3.0, 2.5)));

	//auto light_instance2 = make_shared<material_instance>(light_geom, mat_lib.get("neon_blue"));
	//auto light_instance2_z_rot = make_shared<rotate_z>(light_instance2, -60.0);
	//world.add(make_shared<translate>(light_instance2_z_rot, point3(2.0, 3.0, 2.5)));


	//int num_sphere = 64;
	//for (int i = 0; i < num_sphere; i++) {
	//	float fraction = static_cast<float>(i) / num_sphere;

	//	float y_offset = 7.0f; 

	//	float radius = 5.0f * fraction;
	//	float angle = i * 8.0f;
	//	float height = (fraction * 10.0f - 5.0f) + y_offset;

	//	float x = radius * cos(angle);
	//	float y = height;
	//	float z = -14.0f + radius * sin(angle);
	//	
	//	auto sphere_1 = make_shared<sphere>(point3(x, y, z), 0.3, nullptr);
	//	auto sphere_inst = make_shared<material_instance>(sphere_1, mat_lib.get("white_diffuse"));
	//	world.add(sphere_inst);
	//}

	////cube
	//auto big_cube_geom = make_shared<cube>(point3(0.0, 0.0, 0.0), nullptr);
	//auto big_cube_instance = make_shared<material_instance>(big_cube_geom, mat_lib.get("foggy_glass"));
	//world.add(make_shared<translate>(big_cube_instance, point3(0.0, 1.0, 2.5)));

	//// - 3. RANDOM SPREADED GEOMETRIES
	////
	////master geometries (local prefabicats)
	//auto master_cube = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	//auto master_sphere = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.2, nullptr);
	////material filters
	//auto neon_mats = mat_lib.get_emissive_names();
	//auto regular_mats = mat_lib.get_regular_names();
	////for loop to randomize location of small cubes and spheres*/
	//for (int a = -30; a < 30; a++) {
	//	for (int b = -30; b < 30; b++) { //create the grid a(-15, 15) / b(-15, 15)
	//		point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()); //point3(x,(y = const),z)

	//		if ((center - point3(4.0, 0.2, 0.0)).length() > 0.9) {
	//			std::string selected_mat_name;
	//			shared_ptr<hittable> geometry;
	//			//scale
	//			vec3 scale_v(1.0, 1.0, 1.0); //default scale
	//			//select random number 0.0 - 1.0
	//			double dice_roll = random_double();

	//			//1. Probability distribution
	//			if (dice_roll < 0.25) {
	//				//25% chance to draw neon material
	//				/*int max_idx = static_cast<int>(neon_mats.size()) - 1;*/
	//				/*int idx = random_int(0, static_cast<int>(regular_mats.size()) - 1);*/
	//				selected_mat_name = "white_diffuse";

	//				/*selected_mat_name = neon_mats[random_int(0, max_idx)];*/
	//				geometry = master_cube;
	//				//scale
	//				scale_v = vec3(0.4, random_double(0.4, 1.0), 0.4);
	//			} else if (dice_roll < 0.55) {
	//				//30% chance to draw glass material(0.25 + 0.3 = 0.55)
	//				selected_mat_name = (random_double() < 0.7) ? "glass" : "glass_bubble";
	//				geometry = master_sphere;
	//				double s = random_double(0.5, 2.0);
	//				scale_v = vec3(s, s, s);
	//			} else {
	//				//45% left draw random material left materials from the list (excluding emissive/neaons using regular_mats)
	//				int random_idx = random_int(0, static_cast<int>(regular_mats.size()) - 1);
	//				selected_mat_name = regular_mats[random_idx];

	//				if (random_double() < 0.5) {
	//					geometry = master_sphere;
	//				} else {
	//					geometry = master_cube;
	//				}
	//				//scale to difference
	//				double s = random_double(0.4, 1.5);
	//				scale_v = vec3(s, s, s);

	//			}
	//			//2. get material
	//			auto obj_mat = mat_lib.get(selected_mat_name);

	//			//3. scale
	//			auto scaled_obj = make_shared<scale>(geometry, scale_v);
	//			//4. rotation (only cubes)
	//			shared_ptr<hittable> rotated_obj = scaled_obj;
	//			if (geometry == master_cube) {
	//				rotated_obj = make_shared<rotate_y>(scaled_obj, random_double(0.0, 90.0));
	//			}

	//			//5. applying material to the instance through material_instance class
	//			auto instance = make_shared<material_instance>(rotated_obj, obj_mat);
	//			//3. final position translation
	//			world.add(make_shared<translate>(instance, center));
	//		}
	//	}
	//}

	// - 5. environmental fog
	if (use_fog) {
		//set radius and center of the fog volume (can be adjusted to fit the scene better)
		auto fog_boundary = make_shared<sphere>(point3(0.0, 0.0, 0.0), 50.0, nullptr);
		//fog density 0.1 is extremely high (impenetrable wall). 
		//values 0.001 - 0.02 gives best visual results.
		world.add(make_shared<constant_medium>(fog_boundary, fog_density, fog_color));
	}
	return world;
}