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
	mat_lib.add("wood_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg")));
	mat_lib.add("wood_bumpy_texture", make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"), wood_bump, 8.0));
	mat_lib.add("gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0));
	mat_lib.add("scratched_gold_mat", make_shared<metal>(color(1.0, 0.8, 0.4), 0.0, scratches_bump, -1.0));
	mat_lib.add("mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0));
	mat_lib.add("scratched_mirror", make_shared<metal>(color(1.0, 1.0, 1.0), 0.0, scratches_bump, 1.0));
	mat_lib.add("brushed_aluminium", make_shared<metal>(color(1.0, 1.0, 1.0), 0.25));
	mat_lib.add("black_diffuse", make_shared<lambertian>(color(0.05, 0.05, 0.05)));

	mat_lib.add("metal_colored1", make_shared<metal>(color(random_double(0, 1), random_double(0, 1), random_double(0, 1)), 0.53));
	mat_lib.add("metal_colored2", make_shared<metal>(color(random_double(0, 1), random_double(0, 1), random_double(0, 1)), 0.05));
	mat_lib.add("metal_colored3", make_shared<metal>(color(random_double(0, 1), random_double(0, 1), random_double(0, 1)), 0.15));
	mat_lib.add("metal_colored4", make_shared<metal>(color(random_double(0, 1), random_double(0,1), random_double(0, 1)), 0.75));

	mat_lib.add("white_metal", make_shared<metal>(color(1.0, 1.0, 1.0), 0.99));
	mat_lib.add("white_metal_bump", make_shared<metal>(color(0.9, 0.9, 0.9), 0.6, concrete_bump, 2.0));
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
	mat_lib.add("ceiling_emissive", make_shared<diffuse_light>(color(1.0, 0.0, 0.5) * 10.0));
	//... add more materials as needed

	//floor checker material 
	auto checker = make_shared<checker_texture>(0.5, color(0.1, 0.1, 0.1), color(0.9, 0.9, 0.9));
	mat_lib.add("reflective_checker_mat", make_shared<metal>(checker, 0.02));
}

//build scene geometry
hittable_list build_geometry(MaterialLibrary& mat_lib, const sceneAssetsLoader& assets, bool use_fog, double fog_density, color fog_color) {
	//global material library
	hittable_list world;

	// - 1. FLOOR -
	auto ground_geom = make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, nullptr);
	world.add(make_shared<material_instance>(ground_geom, mat_lib.get("white_diffuse")));

	// 2. DIAGNOSTYCZNA LINIA (Wymusza podziały wzdłuż osi)
	// Tworzymy 8 sfer ułożonych tak, by każda była w innym "narożniku" wirtualnego sześcianu
	for (int i = 0; i < 8; i++) {
		// Rozmieszczenie binarne: (0,0,0), (0,0,1), (0,1,0)... aż do (1,1,1)
		float x = (i & 1) ? 4.0f : -4.0f;
		float y = (i & 2) ? 4.0f : 0.5f;
		float z = (i & 4) ? -10.0f : -18.0f;

		point3 pos(x, y, z);
		auto s_geom = make_shared<sphere>(pos, 0.5, nullptr);

		// Używamy matowego czarnego, by neonowe ramki "płonęły" na krawędziach
		world.add(make_shared<material_instance>(s_geom, mat_lib.get("white_diffuse")));
	}

	// 3. "WIEŻA" (Test podziałów pionowych - oś Y)
	for (int i = 0; i < 5; i++) {
		point3 pos(0, 1.0f + i * 2.0f, -14.0f);
		auto s_geom = make_shared<sphere>(pos, 0.4, nullptr);
		world.add(make_shared<material_instance>(s_geom, mat_lib.get("white_diffuse")));
	}


	//// - 4. LIGHT PLANE 
	///*auto light_geom = make_shared<cube>(point3(-0.2, -0.2, -0.2), point3(0.2, 0.2, 0.2), nullptr);
	//auto light_instance = make_shared<material_instance>(light_geom, mat_lib.get("ceiling_emissive"));
	//world.add(make_shared<translate>(light_instance, point3(0.0, 15.0, 0.0)));*/

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