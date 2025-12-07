#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ray.hpp"
#include "vec3.hpp"
#include "stb_image_write.h"
#include <iostream>
#include <limits>
#include <algorithm>

class camera {
public:
	//image settings
	double aspect_ratio = 1.0;  //ratio of image width over height
	int image_width = 100;      //rendered image width in pixel count
	int samples_per_pixel = 10; //count of random smaples for each pixel
	int max_depth = 10;         //max recursion depth

	//camera settings
	double vfov = 90; //vertical view angle (field of view)
	point3 lookfrom = point3(0, 0, 0); //point where camera is looking from
	point3 lookat = point3(0, 0, -1); //point where camera i s looking at
	vec3 vup = vec3(0, 1, 0); //camera-relative "up" direction

	//defocus blur
	double defocus_angle = 0; //variation angle of rays through each pixel
	double focus_dist = 10; //distance from camera lookfrom point to plane of perfect focus

	//render
	void render(const hittable& world) {
		initialize();

		std::vector<unsigned char> image(image_width * image_height * 3);

		for (int j = 0; j < image_height; j++) {
			std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
			for (int i = 0; i < image_width; i++) {
				color pixel_color(0, 0, 0);
				for (int sample = 0; sample < samples_per_pixel; sample++) {
					ray r = get_ray(i, j);
					pixel_color += ray_color(r, world, max_depth);
				}

				//float color conversion [0,1] -> unsigned char [0,255] with gamma correction
				auto scale = 1.0 / samples_per_pixel;
				auto r_col = std::sqrt(scale * pixel_color.x());
				auto g_col = std::sqrt(scale * pixel_color.y());
				auto b_col = std::sqrt(scale * pixel_color.z());

				int idx = (j * image_width + i) * 3;

				image[idx + 0] = static_cast<unsigned char>(256 * std::clamp(r_col, 0.0, 0.999));
				image[idx + 1] = static_cast<unsigned char>(256 * std::clamp(g_col, 0.0, 0.999));
				image[idx + 2] = static_cast<unsigned char>(256 * std::clamp(b_col, 0.0, 0.999));
			}
		}

		std::clog << "\rDone.                 \n";

		//save to PNG file
		stbi_write_png("image.png", image_width, image_height, 3, image.data(), image_width * 3);

		std::clog << "Image saved to image.png\n";
	}

private:
	int image_height; //rendered image height
	double pixel_samples_scale; //color scale factor for a sum of pixel samples
	point3 center; //camera center
	point3 pixel00_loc; //location of pixel 0, 0
	vec3 pixel_delta_u; //offset to pixel to the right
	vec3 pixel_delta_v; //offset to pixel below
	vec3 u, v, w; //camera frame basis vectors
	vec3 defocus_disk_u; //defocus disk horizntal radius
	vec3 defocus_disk_v; //defocus disk vertical radius

	constexpr static double t_min = 0.001; //min distance (avoid selfcovering)
	constexpr static double t_max = std::numeric_limits<double>::infinity(); //max distance

	void initialize() {
		//calculate the image height , and ensure that it's at least 1
		image_height = int(image_width / aspect_ratio);
		image_height = (image_height < 1) ? 1 : image_height;

		pixel_samples_scale = 1.0 / samples_per_pixel;

		center = lookfrom; //center of the camera source

		//determine viewport dimensions
		auto theta = degrees_to_radians(vfov);
		auto h = std::tan(theta / 2);
		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * (double(image_width) / image_height);

		//calculate the u,v,w unit basis vectors for the camera coordinate frame
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);

		//calculate the vectors across the horizontal and down the vertical viewport edges
		vec3 viewport_u = viewport_width * u; //vector across viewport horizontal edge
		vec3 viewport_v = viewport_height * -v; //vector down viewport vertical edge

		//calculate the horizontal and vertical delta vectors from pixel to pixel
		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		//calculate the location of the upper left pixel
		auto viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;
		pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

		//calculate the camera defcous disk basis vectors
		auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}

	//construct a camera ray originating from the defocus disk and directed at randomly sampled
	//point around the pixel location i, j
	ray get_ray(int i, int j) const {
		auto offset = sample_square();
		auto pixel_sample = pixel00_loc
			+ ((i + offset.x()) * pixel_delta_u)
			+ ((j + offset.y()) * pixel_delta_v);

		auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
		auto ray_direction = pixel_sample - ray_origin;

		return ray(ray_origin, ray_direction);
	}

	//returns the vector to a random point in the [-.5, -.5]-[+.5,+.5] unit square
	vec3 sample_square() const {
		return vec3(random_double() - 0.5, random_double() - 0.5, 0);
	}
	//returns a random point in thecamera defocus disk
	point3 defocus_disk_sample() const {
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	//intersection radius with sphere
	color ray_color(const ray& r, const hittable& world, int depth) const {
		//if we exceeded the ray bounce limit, no more light is gathered
		if (depth <= 0) {
			return color(0, 0, 0); //black color, when reached max depth
		}

		hit_record rec;

		//check if object hit 
		if (world.hit(r, interval(t_min, t_max), rec)) {
			//Ray color with scattered reflectance
			ray scattered;
			color attentuation;
			if (rec.mat->scatter(r, rec, attentuation, scattered)) {
				return attentuation * ray_color(scattered, world, depth - 1);
			}
			return color(0, 0, 0);
		}

		//default blue backfround
		vec3 unit_direction = unit_vector(r.direction());
		auto t = 0.5 * (unit_direction.y() + 1.0);
		//background gradient
		return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
	}
};