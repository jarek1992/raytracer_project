#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ray.hpp"
#include "vec3.hpp"
#include "stb_image_write.h"
#include "hdr_image.hpp"
#include <iostream>
#include <limits>
#include <algorithm>
#include <thread>
#include <functional>
#include <iomanip>

class camera {
public:
	//image settings
	double aspect_ratio = 1.0;  //ratio of image width over height
	int image_width = 100;      //rendered image width in pixel count
	int samples_per_pixel = 10; //count of random smaples for each pixel
	int max_depth = 10;         //max recursion depth
	double sky_intesity = 1.0;    //intensity multiplier for sky color default 1.0

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

		std::vector<color> framebuffer(image_width * image_height);
		std::atomic<int> lines_done = 0;

		//number of threads = number of cores
		int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::thread> threads;

		auto render_rows = [&](int start_y, int end_y) {
			for (int j = start_y; j < end_y; ++j) {
				for (int i = 0; i < image_width; ++i) {
					color pixel_color(0, 0, 0);
					for (int s = 0; s < samples_per_pixel; s++) {
						ray r = get_ray(i, j);
						pixel_color += ray_color(r, world, max_depth);
					}

					framebuffer[j * image_width + i] = pixel_color;
				}

				lines_done++;
			}
			};

		//split lines between threads
		int rows_per_thread = image_height / num_threads;
		int extra = image_height % num_threads;
		int start = 0;

		for (int t = 0; t < num_threads; t++) {
			int end = start + rows_per_thread + (t < extra ? 1 : 0);
			threads.emplace_back(render_rows, start, end);
			start = end;
		}

		//progress bar
		while (lines_done < image_height) {
			int done = lines_done.load();
			double percent = double(done) / image_height;

			int barWidth = 40;
			int filled = int(percent * barWidth);

			std::cerr << "\r[";
			for (int i = 0; i < filled; i++) {
				std::cerr << "#"; //fill of the bar
			}
			for (int i = filled; i < barWidth; i++) {
				std::cerr << "."; //empty space of the bar
			}
			std::cerr << "] ";

			std::cerr << std::fixed << std::setprecision(1)
				<< (percent * 100.0) << "% (" << done << "/" << image_height << " lines)"
				<< std::flush;

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		std::cerr << "\rProgress : 100% (" << image_height << "/" << image_height << " lines)\n";

		//join threads
		for (auto& th : threads)
			th.join();

		std::cerr << "\r[########################################] 100% (" << image_height << "/" << image_height << " lines)\n";

		//conversion framebuffer → RGB
		std::vector<unsigned char> image(image_width * image_height * 3);

		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; i++) {

				color pixel = framebuffer[j * image_width + i];

				//gamma-correction
				double r = std::sqrt(pixel.x() * pixel_samples_scale);
				double g = std::sqrt(pixel.y() * pixel_samples_scale);
				double b = std::sqrt(pixel.z() * pixel_samples_scale);

				int idx = (j * image_width + i) * 3;

				image[idx + 0] = static_cast<unsigned char>(256 * std::clamp(r, 0.0, 0.999));
				image[idx + 1] = static_cast<unsigned char>(256 * std::clamp(g, 0.0, 0.999));
				image[idx + 2] = static_cast<unsigned char>(256 * std::clamp(b, 0.0, 0.999));
			}
		}

		//save to .png
		stbi_write_png("image.png", image_width, image_height, 3, image.data(), image_width * 3);

		std::cerr << "Image saved to image.png\n";
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

	constexpr static double tmin = 0.001; //min distance (avoid selfcovering)
	constexpr static double tmax = std::numeric_limits<double>::infinity(); //max distance

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
		ray cur_ray = r;
		color accumulated_attenuation(1.0, 1.0, 1.0);
		color accumulated_light(0.0, 0.0, 0.0);

		for (int i = 0; i < depth; i++) {
			hit_record rec;
			if (!world.hit(cur_ray, interval(0.001, infinity), rec)) {
				//multiply by sky_intensity to adjust brightness
				color hdr_sample = hdri_image.environment(cur_ray.direction());
				//hit onto background/HDR
				accumulated_light += accumulated_attenuation * (hdr_sample * sky_intesity);
				break;
			}

			ray scattered;
			color attenuation;
			//get emission color from material and pass u, v , p to emitted functions
			color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);

			accumulated_light += accumulated_attenuation * emitted;

			//if material scatters light, return bounce ray color multiplied by attenuation
			if (rec.mat->scatter(cur_ray, rec, attenuation, scattered)) {
				accumulated_attenuation *= attenuation;
				cur_ray = scattered;
			}
			else {
				//material absorbed the ray, no more light is gathered
				break;
			}

			//optimization "russian roulette" or termination of low-contribution rays
			if (accumulated_attenuation.length() < 0.0001) {
				break;
			}
		}
		return accumulated_light;
	}
};