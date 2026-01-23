#include "rtweekend.hpp"

//engine system
#include "camera.hpp"
#include "bvh.hpp"
#include "environment.hpp"
#include "scene_management.hpp"
//stb libraries (to save)
#include "stb_image.h"
#include "stb_image_write.h"

//post-processing


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
	cam.image_width = 800;
	cam.samples_per_pixel = 1000;
	cam.max_depth = 50;
	//b. camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 1.5, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y
	//c. defocus blur settings
	cam.defocus_angle = 0.5; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10.0; //higher values defocus point more far from camera
	//d. render passes
	cam.use_denoiser = true;
	cam.use_albedo_buffer = true;
	cam.use_normal_buffer = true;
	cam.use_z_depth_buffer = true;
	cam.use_reflection = true;
	cam.use_refraction = true;

	// - 6. POST-PROCESSING -
	post_processor my_post;
	//a. parameters
	my_post.exposure = 1.0;
	my_post.contrast = 1.0f;
	my_post.saturation = 1.0f;
	my_post.color_balance = vec3(1.0, 1.0, 1.0);
	my_post.hue_shift = 0.0f; //in degrees [-180, 180]
	my_post.vignette_intensity = 0.0;
	my_post.use_aces_tone_mapping = true;
	//b. z-depth settings
	my_post.z_depth_max_dist = 2.0;
	//c. debug modes
	my_post.use_auto_exposure = true;
	my_post.current_debug_mode = debug_mode::NONE; //change debug mode here(NONE, RED, GREEN, BLUE, LUMINANCE)
	//d. bloom settings
	my_post.use_bloom = true;
	my_post.bloom_threshold = 1.2f;	
	my_post.bloom_intensity = 0.35f;
	my_post.bloom_radius = 4;
	//e. sharpening settings
	my_post.use_sharpening = true;
	my_post.sharpen_amount = 0.1; //0.05 - 0.3 suggested range


	// - 7. RENDER
	cam.render(bvh_world, env, my_post);

	return 0;
}