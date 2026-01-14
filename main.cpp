#include "rtweekend.hpp"

//engine system
#include "camera.hpp"
#include "bvh.hpp"
#include "environment.hpp"
#include "scene_management.hpp"
//stb libraries (to save)
#include "stb_image.h"
#include "stb_image_write.h"

#include <iostream>

int main() {
	// - 1. LOADING MATERIALS FROM THE LIBRARY -
	MaterialLibrary mat_lib;
	load_materials(mat_lib); //from scene_management.hpp

	// - 2. LOADING THE GEOMETRY -
	hittable_list world = build_geometry(mat_lib); //from scene_management.hpp

	// - 3. BVH ACCELERATION STRUCTURE -
	hittable_list bvh_world;
	bvh_world.add(make_shared<bvh_node>(world));

	// - 4. ENVIRONMENT SETTINGS
	EnvironmentSettings env;
	//set environment mode to 1. HDRI or 2. PHYSICAL SUN Model
	//a. HDR map mode:
	//auto hdr_tex = make_shared<image_texture>("assets/sunny_rose_garden_2k.hdr", true);
	//env.mode = EnvironmentSettings::HDR_MAP;
	//env.hdr_texture = hdr_tex; //load HDR texture from file
	//env.intensity = 1.0;
	//env.hdri_rotation = degrees_to_radians(-45.0); //rotate environment map around Y-axis
	//env.hdri_tilt = degrees_to_radians(-4.0); //tilt environment map around X-axis (horizont height)

	//b. physical sun with sky model: 
	env.mode = EnvironmentSettings::PHYSICAL_SUN;
	env.sun_direction = unit_vector(vec3(-0.4, -0.3, -0.2));
	env.sun_color = color(0.8, 0.1, 0.1);
	env.sun_intensity = 1.0;
	env.sun_size = 5000.0; //higher value makes sun disc smaller
	env.intensity = 0.2;

	// - 5. CREATE CAMERA AND SET PARAMETERS -
	camera cam;
	//a. image aspects ratio
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 600;
	cam.samples_per_pixel = 150;
	cam.max_depth = 50;
	//b. camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 1.5, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y
	//c. defocus blur settings
	cam.defocus_angle = 0.6; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10.0; //higher values defocus point more far from camera
	//d. render passes
	cam.use_denoiser = true;
	cam.use_albedo_buffer = true;
	cam.use_normal_buffer = true;
	cam.use_z_depth_buffer = true;

	// - 6. RENDER
	cam.render(bvh_world, env);

	return 0;
}