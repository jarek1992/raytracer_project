#include "stb_image.h"
#include "stb_image_write.h"
#include "rtweekend.hpp"
#include "camera.hpp"
#include "hittable.hpp"
#include "hittable_list.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "cube.hpp"
#include "rotate_x.hpp"
#include "rotate_y.hpp"
#include "rotate_z.hpp"
#include "translate.hpp"
#include "triangle.hpp"
#include "bvh.hpp"
#include "material_instance.hpp"
#include "model.hpp"
#include "environment.hpp"
#include "material_library.hpp"
#include "scale.hpp"

#include <map>
#include <string>
#include <vector>
#include <algorithm>

int main() {
	//global material library
	MaterialLibrary mat_lib;
	
	//add some predefined materials to the library
	mat_lib.add("red_diffuse", make_shared<lambertian>(color(0.8, 0.1, 0.1)));
	mat_lib.add("rough_gold", make_shared<metal>(color(1.0, 0.84, 0.0), 0.15));
	mat_lib.add("glass", make_shared<dielectric>(1.5));
	mat_lib.add("light_blue_diffuse", make_shared<lambertian>(color(0.1, 0.4, 0.9)));
	mat_lib.add("white_diffuse", make_shared<lambertian>(color(0.9, 0.9, 0.9)));
	mat_lib.add("wood_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg")));
	mat_lib.add("gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0));
	mat_lib.add("mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("brushed_aluminium", make_shared<metal>(color(1.0, 1.0, 1.0), 0.25));
	mat_lib.add("metal_colored", make_shared<metal>(color(0.2, 0.8, 0.2), 0.05));
	mat_lib.add("checker_texture", make_shared<lambertian>(make_shared<checker_texture>(0.5, color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9))));
	mat_lib.add("glass_bubble", make_shared<dielectric>(1.0 / 1.5));
	mat_lib.add("pure_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("random_diffuse", make_shared<lambertian>(color::random() * color::random()));
	mat_lib.add("random_neon_light", make_shared<diffuse_light>(color::random(0.1, 1.0) * 1.5));
	mat_lib.add("neon_pink", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 6.0));
	mat_lib.add("neon_blue", make_shared<diffuse_light>(color(0.0, 0.5, 1.0) * 6.0));
	mat_lib.add("neon_green", make_shared<diffuse_light>(color(0.1, 1.0, 0.1) * 6.0));
	mat_lib.add("neon_yellow", make_shared<diffuse_light>(color(1.0, 0.8, 0.0) * 6.0));
	mat_lib.add("neon_white", make_shared<diffuse_light>(color(1.0, 1.0, 1.0) * 6.0));
	mat_lib.add("neon_red", make_shared<diffuse_light>(color(1.0, 0.1, 0.1) * 6.0));
	mat_lib.add("ceiling_light", make_shared<diffuse_light>(color(1.0, 1.0, 1.0) * 1.2));
	//... add more materials as needed

	//set up a sphere into world
	hittable_list world;

	//0. BASE MATERIAL AND GROUND PLANE
	//define ground color (dark grey and light grey)
	auto dark_grey = color(0.1, 0.1, 0.1);
	auto light_grey = color(0.9, 0.9, 0.9);
	//checker texture for ground plane(0.5 scale determine size of squares)
	auto checker = make_shared<checker_texture>(0.5, dark_grey, light_grey);
	//combine texture with metal material
	//fuzz 0.00 for perfect reflection
	auto reflective_checker_mat = make_shared<metal>(checker, 0.02);
	world.add(make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, reflective_checker_mat));

	//SOFT LIGHT ABOVE
	auto light_geom = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	auto light_instance = make_shared<material_instance>(light_geom, mat_lib.get("ceiling_light"));
	world.add(make_shared<translate>(light_instance, point3(0.0, 15.0, 0.0)));

	//GEOMETRIES WITH MATERIAL INSTANCING
	//TEAPOT
	auto teapot_base = make_shared<model>("assets/models/teapot.obj", nullptr, 0.4);
	auto teapot_inst = make_shared<material_instance>(teapot_base, mat_lib.get("wood_texture"));
	auto rot_teapot_x = make_shared<rotate_x>(teapot_inst, -90.0);
	auto rot_teapot_y = make_shared<rotate_y>(rot_teapot_x, 30.0);
	auto teapot_final = make_shared<translate>(rot_teapot_y, point3(0.0, 1.0, -2.5));
	world.add(teapot_final);
	//SPHERE
	auto big_sphere_geom = make_shared<sphere>(point3(0.0, 0.0, 0.0), 1.0, nullptr);
	auto big_sphere_instance = make_shared<material_instance>(big_sphere_geom, mat_lib.get("brushed_aluminium"));
	world.add(make_shared<translate>(big_sphere_instance, point3(0.0, 1.0, 0.0)));
	//SMALLER SPHERE GLASS
	auto small_sphere_geom = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.5, nullptr);
	auto small_sphere_instance = make_shared<material_instance>(small_sphere_geom, mat_lib.get("glass"));
	world.add(make_shared<translate>(small_sphere_instance, point3(3.0, 0.5, -1.0)));
	//BUBBLE SPHERE
	auto small_bubble_instance = make_shared<material_instance>(small_sphere_geom, mat_lib.get("glass_bubble"));
	world.add(make_shared<translate>(small_bubble_instance, point3(3.0, 0.5, 1.0)));
	//CUBE
	auto big_cube_geom = make_shared<cube>(point3(0.0, 0.0, 0.0), nullptr);
	auto big_cube_instance = make_shared<material_instance>(big_cube_geom, mat_lib.get("neon_red"));
	world.add(make_shared<translate>(big_cube_instance, point3(0.0, 1.0, 2.5)));

	//7. SINGLE MASTER CUBE AND SPHERE FOR INSTANCING*/
	auto master_cube = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	auto master_sphere = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.2, nullptr);

	//get a list of all material names from the material library
	std::vector<std::string> mat_names = mat_lib.get_material_names(); //vector to hold different materials for instances
	
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
					selected_mat_name = neon_mats[random_int(0, static_cast<int>(neon_mats.size()) - 1)];
					geometry = master_cube;
					//scale
					scale_v = vec3(0.4, random_double(1.5, 4.5), 0.4);
				} else if (dice_roll < 0.55) {
					//30% chance to draw glass material(0.25 + 0.3 = 0.55)
					selected_mat_name = (random_double() < 0.7) ? "glass" : "glass_bubble";
					geometry = master_sphere;
					double s = random_double(0.5, 1.0);
					scale_v = vec3(s, s, s);
				} else {
					//45% left draw random material left materials from the list (excluding emissive/neaons using regular_mats)
					int random_idx = random_int(0, static_cast<int>(regular_mats.size()) - 1);
					selected_mat_name = regular_mats[random_idx];

					if (random_double() < 0.5) {
						geometry = master_sphere;
					} else {
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

	//create camera
	camera cam;

	//image aspects ratio
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 500;
	cam.samples_per_pixel = 150;
	cam.max_depth = 80;

	//camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 2, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y

	//defocus blur settings
	cam.defocus_angle = 0.6; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10.0; //higher values defocus point more far from camera
	
	//environment settings
	EnvironmentSettings env;

	//set environment mode to 1. HDRI or 2. PHYSICAL SUN Model
	// 
	////1. HDR MAP MODE
	//auto hdr_tex = make_shared<image_texture>("assets/sunny_rose_garden_2k.hdr", true);
	//env.mode = EnvironmentSettings::HDR_MAP;
	//env.hdr_texture = hdr_tex; //load HDR texture from file
	//env.intensity = 1.0;
	//env.hdri_rotation = degrees_to_radians(-45.0); //rotate environment map around Y-axis
	//env.hdri_tilt = degrees_to_radians(-4.0); //tilt environment map around X-axis (horizont height)

	//2. PHYSICAL SUN with SKY MODEL and settings: 
	env.mode = EnvironmentSettings::PHYSICAL_SUN;
	env.sun_direction = unit_vector(vec3(-0.4, -0.25, -0.2));
	env.sun_color = color(1.0, 1.0, 1.0);
	env.sun_intensity = 0.7;
	env.sun_size = 5000.0; //higher value makes sun disc smaller
	env.intensity = 1.0;
	
	//bvh acceleration structure
	hittable_list bvh_world;
	bvh_world.add(make_shared<bvh_node>(world));

	//render
	cam.render(bvh_world, env);

	return 0;
}