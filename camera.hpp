#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ray.hpp"
#include "vec3.hpp"
#include "stb_image_write.h"
#include "environment.hpp"
#include <OpenImageDenoise/oidn.hpp>

#include <iostream>
#include <limits>
#include <algorithm>
#include <thread>
#include <functional>
#include <iomanip>
#include <chrono>

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

	//denoiser flag
	bool use_denoiser = true;

	//render
	void render(const hittable& world, const EnvironmentSettings& env) {
		// - 1. INITIALIZE - 
		initialize();
		//timer for render start
		auto start_time = std::chrono::high_resolution_clock::now();

		// - 2. MULTITHREADING 
		std::vector<color> framebuffer(image_width * image_height);
		execute_render_threads(world, env, framebuffer);
		
		// - 3. AI DENOISING
		if (use_denoiser) {
			std::cerr << "\rApplying AI Denoising (Intel OIDN)..." << std::flush;
			apply_denoising(image_width, image_height, framebuffer);
		} else {
			std::cerr << "\nDenoising is DISABLED. Skipping..." << std::endl;
		}

		// - 4. POST-PROCESSING (tone mapping & save)
		process_framebuffer_to_image(framebuffer);

		//timer for render end
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end_time - start_time;

		std::cerr << "\r[########################################] 100% (" << image_height << "/" << image_height << " lines)\n";
		std::cerr << "\nRender finished in " << std::fixed << std::setprecision(2) << elapsed.count() << " seconds.\n";
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

	void execute_render_threads(const hittable& world, const EnvironmentSettings& env, std::vector<color>& framebuffer) {
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
						pixel_color += ray_color(r, world, max_depth, env);
					}
					framebuffer[j * image_width + i] = pixel_color;
				}
				lines_done++;
			}
			};

		//split the work between threads
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
		//final 100% while loop finished
		std::cerr << "\rProgress : 100% (" << image_height << "/" << image_height << " lines)\n";

		//join threads - finished
		for (auto& th : threads)
		{
			th.join();
		}
	}

	void apply_denoising(int width, int height, std::vector<color>& framebuffer) {
		//OIDN requires float type data, copy if 'color' is double 
		std::vector<float> float_buffer(width * height * 3);
		for (size_t i = 0; i < framebuffer.size(); ++i) {
			float_buffer[i * 3 + 0] = static_cast<float>(framebuffer[i].x());
			float_buffer[i * 3 + 1] = static_cast<float>(framebuffer[i].y());
			float_buffer[i * 3 + 2] = static_cast<float>(framebuffer[i].z());
		}

		//initialize OIDN tool
		std::cerr << "\rApplying AI Denoising (Intel OIDN)..." << std::endl;
		oidn::DeviceRef device = oidn::newDevice();
		device.commit();

		//check erros
		const char* errorMessage;
		if (device.getError(errorMessage) != oidn::Error::None) {
			std::cerr << "ERROR OIDN: " << errorMessage << std::endl;
			return;
		}

		//filter
		std::cerr << "Applying filters..." << std::endl;
		oidn::FilterRef filter = device.newFilter("RT"); // RT to skrót od Ray Tracing
		filter.setImage("color", float_buffer.data(), oidn::Format::Float3, width, height);
		filter.setImage("output", float_buffer.data(), oidn::Format::Float3, width, height);
		filter.set("hdr", true); // Bardzo ważne dla neonów i słońca!
		filter.commit();

		//AI execution
		filter.execute();

		std::cerr << "Denoising finished." << std::endl;

		//copying the results back to the framebuffer (from float to double)
		for (size_t i = 0; i < framebuffer.size(); ++i) {
			framebuffer[i] = color(float_buffer[i * 3 + 0],
				float_buffer[i * 3 + 1],
				float_buffer[i * 3 + 2]);
		}
	}

	void process_framebuffer_to_image(const std::vector<color>& framebuffer) {
		//conversion framebuffer → RGB
		std::vector<unsigned char> image(image_width * image_height * 3);

		//tone mapping and RGB conversion
		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; i++) {
				//get raw HDR color and scale by sample amount
				color raw_color = framebuffer[j * image_width + i] * pixel_samples_scale;

				//exposure - tone down or brighten up the scene overall
				double exposure = 1.0;
				raw_color *= exposure;

				//ACES tone mapping - avoid to overburn the scene lights to pure white too fast
				auto aces_color = [](color x) {
					double a = 2.51;
					double b = 0.03;
					double c = 2.43;
					double d = 0.59;
					double e = 0.14;
					return color(
						(x.x() * (a * x.x() + b)) / (x.x() * (c * x.x() + d) + e),
						(x.y() * (a * x.y() + b)) / (x.y() * (c * x.y() + d) + e),
						(x.z() * (a * x.z() + b)) / (x.z() * (c * x.z() + d) + e)
					);
					};

				color mapped = aces_color(raw_color);

				//gamma-correction
				double r = std::pow(mapped.x(), 1.0 / 2.2);
				double g = std::pow(mapped.y(), 1.0 / 2.2);
				double b = std::pow(mapped.z(), 1.0 / 2.2);

				int idx = (j * image_width + i) * 3;
				image[idx + 0] = static_cast<unsigned char>(255 * std::clamp(r, 0.0, 0.999));
				image[idx + 1] = static_cast<unsigned char>(255 * std::clamp(g, 0.0, 0.999));
				image[idx + 2] = static_cast<unsigned char>(255 * std::clamp(b, 0.0, 0.999));
			}
		}

		//save to .png
		stbi_write_png("image.png", image_width, image_height, 3, image.data(), image_width * 3);
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

	color get_background_color(const ray& r, const EnvironmentSettings& env) const {
		vec3 unit_dir = unit_vector(r.direction());
		color env_color;

		if (env.mode == EnvironmentSettings::HDR_MAP && env.hdr_texture) {
			vec3 d = unit_dir;

			//HDR rotation 
			double cos_y = cos(env.hdri_rotation);
			double sin_y = sin(env.hdri_rotation);
			double x1 = cos_y * d.x() - sin_y * d.z();
			double z1 = sin_y * d.x() + cos_y * d.z();
			d = vec3(x1, d.y(), z1);

			double cos_t = cos(env.hdri_tilt);
			double sin_t = sin(env.hdri_tilt);
			double y2 = cos_t * d.y() - sin_t * d.z();
			double z2 = sin_t * d.y() + cos_t * d.z();
			d = vec3(d.x(), y2, z2);

			//uv mapping
			auto phi = atan2(d.z(), d.x()) + pi;
			auto theta = acos(std::clamp(d.y(), -1.0, 1.0));

			env_color = env.hdr_texture->value(phi / (2 * pi), theta / pi, point3(0, 0, 0));
		}
		else {
			//physcial sun model
			double sun_height = env.sun_direction.y();
			double day_factor = std::clamp(sun_height * 4.0, 0.0, 1.0);
			double sunset_factor = std::clamp(1.0 - std::abs(sun_height - 0.1) * 5.0, 0.0, 1.0);

			color zenit_color = color(0.1, 0.2, 0.5) * (1.0 - day_factor) + color(0.4, 0.6, 1.0) * day_factor;
			color horizon_color = color(0.1, 0.05, 0.01) * (1.0 - day_factor) + color(0.8, 0.9, 1.0) * day_factor;
			horizon_color = horizon_color * (1.0 - sunset_factor) + color(1.0, 0.4, 0.1) * sunset_factor;

			auto a = 0.5 * (unit_dir.y() + 1.0);
			env_color = (1.0 - a) * horizon_color + a * zenit_color;

			double sun_focus = dot(unit_dir, env.sun_direction);
			if (sun_focus > 0.0 && sun_height > -0.1) {
				color current_sun_color = env.sun_color * (1.0 - sunset_factor) + color(1.0, 0.3, 0.1) * sunset_factor;
				double visibility = std::clamp(sun_height * 10.0 + 0.5, 0.0, 1.0);
				env_color += current_sun_color * std::pow(sun_focus, env.sun_size) * env.sun_intensity * visibility;
			}
		}
		return env_color * env.intensity;
	}

	//intersection radius with sphere
	color ray_color(const ray& r, const hittable& world, int depth, const EnvironmentSettings& env) const {
		color accumulated_light(0, 0, 0);
		color accumulated_attenuation(1, 1, 1);
		ray cur_ray = r;

		for (int i = 0; i < depth; i++) {
			hit_record rec;

			if (!world.hit(cur_ray, interval(0.001, infinity), rec)) {
				accumulated_light += accumulated_attenuation * get_background_color(cur_ray, env);
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

