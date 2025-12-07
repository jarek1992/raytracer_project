#include "rtweekend.hpp"
#include "camera.hpp"
#include "hittable.hpp"
#include "hittable_list.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "cube.hpp"

int main() {
	//set up a sphere into world
	hittable_list world;

	//base material and plane
	auto material_ground = make_shared<metal>(color(0.2, 0.2, 0.2), 0.3);
	world.add(make_shared<sphere>(point3(0.0, -1000.0, 0.0), 1000.0, material_ground));

	//glass sphere into scene
	auto material_glass = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 1.0, material_glass));

	//bubble sphere into scene
	auto material_bubble = make_shared<dielectric>(1.0 / 1.5);
	world.add(make_shared<sphere>(point3(0.0, 1.0, 0.0), 0.9, material_bubble));

	//cube 
	auto cube_material = make_shared<lambertian>(color(0.2, 0.5, 0.5));
	point3 min_corner(2.0, 0.0, 2.0);
	point3 max_corner(4.0, 2.0, 4.0);

	double angle_deg = 0.0;
	double angle_rad = degrees_to_radians(angle_deg);

	world.add(make_shared<cube>(min_corner, max_corner, cube_material, angle_rad));

	//metal material sphere into scene
	auto material_metal = make_shared<metal>(color(0.2, 0.2, 0.2), 0.2);
	world.add(make_shared<sphere>(point3(6.0, 1.0, -2.0), 1.0, material_metal));

	//randomize location of small spheres and cubes
	for (int a = -20; a < 20; a++) {
		for (int b = -20; b < 20; b++) { //create the grid a(-20, 20) / b(-20, 20)
			auto choose_material = random_double(); //randomize material for each sphere/cube in range(0, 1)
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double()); //point3(x,(y = const),z)

			if ((center - point3(5.0, 1.0, 0.0)).length() > 0.9) { //condition to create sphere or cube
				shared_ptr<material> object_material;

				if (choose_material < 0.8) {
					//diffuse material
					auto albedo = color::random() * color::random(); //random color
					object_material = make_shared<lambertian>(albedo);

					if (random_double() < 0.5) {
						//crete sphere
						world.add(make_shared<sphere>(center, 0.2, object_material));
					}
					else {
						//create cube
						point3 min_corner(center.x() - 0.2, center.y() - 0.2, center.z() - 0.2);
						point3 max_corner(center.x() + 0.2, center.y() + 0.2, center.z() + 0.2);

						double angle_deg = random_double(0.0, 360.0);
						double angle_rad = degrees_to_radians(angle_deg);

						world.add(make_shared<cube>(min_corner, max_corner, object_material, angle_rad));
					}
				}
				else if (choose_material < 0.95) {
					//metal material
					auto albedo = color::random(0.5, 1); //range (0.5, 1) allows to select amongst brighter range
					auto fuzz = random_double(0, 0.5); //higher value more diffused reflections
					object_material = make_shared<metal>(albedo, fuzz);

					if (random_double() < 0.5) {
						//crete sphere
						world.add(make_shared<sphere>(center, 0.2, object_material));
					}
					else {
						//create cube
						point3 min_corner(center.x() - 0.2, center.y() - 0.2, center.z() - 0.2);
						point3 max_corner(center.x() + 0.2, center.y() + 0.2, center.z() + 0.2);

						double angle_deg = random_double(0.0, 360.0);
						double angle_rad = degrees_to_radians(angle_deg);

						world.add(make_shared<cube>(min_corner, max_corner, object_material, angle_rad));
					}
				}
				else {
					//glass material
					object_material = make_shared<dielectric>(1.5);

					if (random_double() < 0.5) {
						//crete sphere
						world.add(make_shared<sphere>(center, 0.2, object_material));
					}
					else {
						//create cube
						point3 min_corner(center.x() - 0.2, center.y() - 0.2, center.z() - 0.2);
						point3 max_corner(center.x() + 0.2, center.y() + 0.2, center.z() + 0.2);

						double angle_deg = random_double(0.0, 360.0);
						double angle_rad = degrees_to_radians(angle_deg);

						world.add(make_shared<cube>(min_corner, max_corner, object_material, angle_rad));
					}
				}
			}
		}
	}

	//create camera
	camera cam;

	//image aspects ratio
	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 400;
	cam.samples_per_pixel = 50;
	cam.max_depth = 10;

	//camera settings
	cam.vfov = 30; //vertical field of the view
	cam.lookfrom = point3(10, 2, 0); //cords for camera source
	cam.lookat = point3(0, 0, 0); //cords for camera target
	cam.vup = vec3(0, 1, 0); //up vector set to Y

	//defocus blur settings
	cam.defocus_angle = 0.0; //higher values more blur on objects outside defocus point
	cam.focus_dist = 10; //higher values defocus point more far from camera

	//render the scene
	cam.render(world);

	return 0;
}