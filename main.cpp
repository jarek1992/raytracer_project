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

int main() {
	//set up a sphere into world
	hittable_list world;

	//0. base material and plane
	auto material_ground = make_shared<metal>(color(0.2, 0.2, 0.2), 0.3);
	
	world.add(make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, material_ground));

	//1. glass sphere into scene
	auto material_glass = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 1.0, material_glass));

	//2. bubble sphere into scene
	auto material_bubble = make_shared<dielectric>(1.0 / 1.5);
	
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 0.9, material_bubble));

	//3. cube 
	/*auto cube_material = make_shared<lambertian>(color(0.2, 0.5, 0.5));*/
	auto texture = make_shared<image_texture>("assets/textures/fine-wood.jpg");
	auto textured_lambertian = make_shared<lambertian>(texture);
	auto cube1 = make_shared<cube>(point3(0.0, 0.0, 0.0), textured_lambertian);
	auto rotated = make_shared<rotate_y>(cube1, 30.0);
	auto final_pos = make_shared<translate>(rotated, point3(3.0, 1.0, 3.0));
	
	world.add(final_pos);

	//4. triangle
	auto triangle_material = make_shared<lambertian>(color(0.8, 0.3, 0.3));
	point3 v0(-1.0, 0.0, 0.0);
	point3 v1(1.0, 0.0, 0.0);
	point3 v2(0.0, 2.0, 0.0);
	auto triangle1 = make_shared<triangle>(v0, v1, v2, triangle_material);
	auto triangle_rotated = make_shared<rotate_y>(triangle1, 45.0);
	auto triangle_final_pos = make_shared<translate>(triangle_rotated, point3(-2.0, 0.0, 1.0));

	world.add(triangle_final_pos);

	//5. metal material sphere into scene
	//emmissive material
	auto emmissive_material = make_shared<diffuse_light>(color(15.0, 15.0, 15.0)); //bright white light
	/*auto material_metal = make_shared<metal>(color(0.2, 0.2, 0.2), 0.2);*/
	world.add(make_shared<sphere>(point3(6.0, 1.0, -2.0), 1.0, emmissive_material));

	//common materials for small cubes
	auto mat_diffuse = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	auto mat_wood = make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"));
	//6. single cube in the scene
	//centered at (0.0, 0.0, 0.0) 
	auto master_cube = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), mat_diffuse);

	//for loop to randomize location of small cubes
	for (int a = -20; a < 20; a++) {
		for (int b = -20; b < 20; b++) { //create the grid a(-20, 20) / b(-20, 20)
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()); //point3(x,(y = const),z)

			if ((center - point3(5.0, 1.0, 0.0)).length() > 0.9) { //condition to create cube
				//instance of cube
				//using rotate_y and translate as a wrapper for master_cube
				auto rotated_cube = make_shared<rotate_y>(master_cube, random_double(0.0, 90.0));
				auto final_instance = make_shared<translate>(rotated_cube, center);

				//add to world
				world.add(final_instance);
			}
		}
	}

	//create camera
	camera cam;

	//image aspects ratio
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 800;
	cam.samples_per_pixel = 100;
	cam.max_depth = 30;

	//camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 2, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y

	//defocus blur settings
	cam.defocus_angle = 0.6; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10; //higher values defocus point more far from camera


	if (!hdri_image.load("assets/sunny_rose_garden_2k.hdr")) {
		std::cerr << "Failed to load HDRI, continuing with black sky\n";
	}

	//rotate HDRI Yaw and tilt
	HDRI_ROTATION = degrees_to_radians(60.0);
	HDRI_TILT = degrees_to_radians(-3.0);

	//render the scene
	hittable_list bvh_world;
	bvh_world.add(make_shared<bvh_node>(world));
	cam.render(bvh_world);

	return 0;
}