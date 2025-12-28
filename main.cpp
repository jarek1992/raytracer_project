#include "stb_image.h"
#include "stb_image_write.h"
#include "rtweekend.hpp"
#include "camera.hpp"
#include "hittable.hpp"
#include "hittable_list.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "cube.hpp"
#include "hdr_image.hpp"
#include "rotate_y.hpp"
#include "translate.hpp"
#include "triangle.hpp"
#include "bvh.hpp"
#include "material_instance.hpp"

#include <map>
#include <string>

//materials library
std::map<std::string, shared_ptr<material>> material_library;

//additional function for adding materials to library
void add_material(const std::string& name, shared_ptr<material> mat) {
	material_library[name] = mat;
}

int main() {
	//set up a sphere into world
	hittable_list world;

	//0. BASE MATERIAL AND GROUND PLANE
	//define ground color (dark grey and light grey)
	auto dark_grey = color(0.1, 0.1, 0.1);
	auto light_grey = color(0.9, 0.9, 0.9);
	//checker texture for ground plane(0.5 scale determine size of squares)
	auto checker = make_shared<checker_texture>(0.5, dark_grey, light_grey);
	//combine texture with metal material
	//fuzz 0.0 for perfect reflection
	auto reflective_checker_mat = make_shared<metal>(checker, 0.02);

	world.add(make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, reflective_checker_mat));
	

	//1. GLASS SPHERE into scene
	auto material_glass = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 1.0, material_glass));

	//2. BUBBLE INSIDE GLASS SPHERE
	auto material_bubble = make_shared<dielectric>(1.0 / 1.5);
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 0.9, material_bubble));

	//3. CUBE
	/*auto cube_material = make_shared<lambertian>(color(0.2, 0.5, 0.5));*/
	auto texture = make_shared<image_texture>("assets/textures/fine-wood.jpg");
	auto textured_lambertian = make_shared<lambertian>(texture);
	auto cube1 = make_shared<cube>(point3(0.0, 0.0, 0.0), textured_lambertian);
	auto rotated = make_shared<rotate_y>(cube1, 30.0);
	auto final_pos = make_shared<translate>(rotated, point3(3.0, 1.0, 3.0));
	world.add(final_pos);

	//4. TRIANGLE
	auto triangle_material = make_shared<lambertian>(color(0.8, 0.3, 0.3));
	point3 v0(-1.0, 0.0, 0.0);
	point3 v1(1.0, 0.0, 0.0);
	point3 v2(0.0, 2.0, 0.0);
	auto triangle1 = make_shared<triangle>(v0, v1, v2, triangle_material);
	auto triangle_rotated = make_shared<rotate_y>(triangle1, 45.0);
	auto triangle_final_pos = make_shared<translate>(triangle_rotated, point3(-2.0, 0.0, 1.0));
	world.add(triangle_final_pos);

	//5. EMMISIVE SPHERE 
	//emmissive material
	color neon_sphere = (random_double() < 0.5) ? color(0.1, 0.9, 1.0) : color(1.0, 0.1, 0.8);
	auto emmissive_material = make_shared<diffuse_light>(neon_sphere * 7.0); //intensity 7.0

	world.add(make_shared<sphere>(point3(6.0, 1.0, -2.0), 1.0, emmissive_material));
	/*auto material_metal = make_shared<metal>(color(0.2, 0.2, 0.2), 0.2);*/
	//common materials for small cubes
	/*auto mat_diffuse = make_shared<lambertian>(color(0.5, 0.5, 0.5));*/
	/*auto mat_wood = make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"));*/

	//6. SINGLE MASTER CUBE AND SPHERE FOR INSTANCING
	auto master_cube = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	auto master_sphere = make_shared<sphere>(point3(0.0, 0.0, 0.0), 0.2, nullptr);

	//for loop to randomize location of small cubes and spheres
	for (int a = -15; a < 15; a++) {
		for (int b = -15; b < 15; b++) { //create the grid a(-15, 15) / b(-15, 15)
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()); //point3(x,(y = const),z)

			if ((center - point3(4.0, 0.2, 0.0)).length() > 0.9) { //condition to create cube
				shared_ptr<material> obj_mat;
				double choose_mat = random_double();

				//1. material selection
				if (choose_mat < 0.12) {
					//neons (emmissive) 12% chance - highly saturated colors
					color neon_color = (random_double() < 0.5) ? color(0.1, 0.9, 1.0) : color(1.0, 0.1, 0.8);
					obj_mat = make_shared<diffuse_light>(neon_color * 8.0); //intensity 10.0
				
					auto instance = make_shared<material_instance>(master_sphere, obj_mat);
					world.add(make_shared<translate>(instance, center));

					continue; //skip rest of loop to avoid adding another object
				} else if (choose_mat < 0.6) {
					//diffuse (matte)
					auto albedo = color::random() * color::random();
					obj_mat = make_shared<lambertian>(albedo);
				} else if(choose_mat < 0.85) {
					//metal
					auto albedo = color::random(0.5, 1.0);
					auto fuzz = random_double(0.0, 0.3);
					obj_mat = make_shared<metal>(albedo, fuzz);
				}
				else {
					//glass
					obj_mat = make_shared<dielectric>(1.5);
				}

				//2. object selection (cube or sphere 50/50)
				shared_ptr<hittable> geometry;
				if (random_double() < 0.5) {
					//sphere instance
					geometry = master_sphere;
				} else {
					//cube instance with random rotation_y
					geometry = make_shared<rotate_y>(master_cube, random_double(0.0, 90.0));
				}

				//applying material to the instance through material_instance class
				auto instance = make_shared<material_instance>(geometry, obj_mat);

				//3. final position translation
				world.add(make_shared<translate>(instance, center));
			}
		}
	}

	//create camera
	camera cam;

	//image aspects ratio
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 800;
	cam.samples_per_pixel = 300;
	cam.max_depth = 50;

	//camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 2, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y

	//HDRI sky settings
	cam.sky_intesity = 0.1; //intensity multiplier for sky color

	//defocus blur settings
	cam.defocus_angle = 0.6; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10.0; //higher values defocus point more far from camera


	if (!hdri_image.load("assets/sunny_rose_garden_2k.hdr")) {
		std::cerr << "Failed to load HDRI, continuing with black sky\n";
	}

	//rotate HDRI Yaw and tilt
	HDRI_ROTATION = degrees_to_radians(-45.0);
	HDRI_TILT = degrees_to_radians(-4.0);

	//render the scene
	hittable_list bvh_world;
	bvh_world.add(make_shared<bvh_node>(world));
	cam.render(bvh_world);

	return 0;
}