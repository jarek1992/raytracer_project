#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ray.hpp"
#include "vec3.hpp"
#include "stb_image_write.h"
#include "environment.hpp"
#include "color_processing.hpp"
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

	//render passes
	bool use_albedo_buffer = false;
	bool use_normal_buffer = false;
	bool use_z_depth_buffer = false;

	//render
	void render(const hittable& world, const EnvironmentSettings& env, const post_processor& post) {
		// - 1. INITIALIZE - 
		initialize();
		//timer for render start
		auto start_time = std::chrono::high_resolution_clock::now();

		// - 2. MULTITHREADING 
		std::vector<color> framebuffer(image_width * image_height);
		std::vector<color> albedo_buffer(image_width * image_height);
		std::vector<color> normal_buffer(image_width * image_height);
		std::vector<color> z_depth_buffer(image_width * image_height);
		execute_render_threads(world, env, framebuffer, albedo_buffer, normal_buffer, z_depth_buffer, post.z_depth_max_dist);

		// - 3. SAVE RAW RENDER RGB
		process_framebuffer_to_image(framebuffer, "image_RGB_raw.png", post, false);

		// - 4. AI DENOISING
		if (use_denoiser) {
			apply_denoising(image_width, image_height, framebuffer, albedo_buffer, normal_buffer);

			//save with denoiser only if use_denoiser == true
			process_framebuffer_to_image(framebuffer, "image_RGB_denoised.png", post, false);
		}
		else {
			std::cerr << "\nDenoising is DISABLED. Skipping..." << std::endl;
		}

		// - 5. POST-PROCESSING (tone mapping & save)
		//
		//render passes without tone mapping
		if (use_albedo_buffer) {
			process_framebuffer_to_image(albedo_buffer, "image_albedo.png", post, true);
		}
		if (use_normal_buffer) {
			process_framebuffer_to_image(normal_buffer, "image_normals.png", post, true);
		}
		if (use_z_depth_buffer) {
			process_framebuffer_to_image(z_depth_buffer, "image_zdepth.png", post, true);
		}

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

	void execute_render_threads(
		const hittable& world,
		const EnvironmentSettings& env,
		std::vector<color>& framebuffer,
		std::vector<color>& albedo_buffer,
		std::vector<color>& normal_buffer,
		std::vector<color>& z_depth_buffer,
		double z_depth_max_dist) {
		std::atomic<int> lines_done = 0;

		//number of threads = number of cores
		int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::thread> threads;

		auto render_rows = [&](int start_y, int end_y) {
			const int aux_sample = 64;
			const double aux_scale = 1.0 / aux_sample;

			for (int j = start_y; j < end_y; ++j) {
				for (int i = 0; i < image_width; ++i) {
					color pixel_color(0, 0, 0);
					color pixel_albedo(0, 0, 0);
					color pixel_normal(0, 0, 0);
					color pixel_zdepth(0, 0, 0);

					for (int s = 0; s < samples_per_pixel; s++) {
						//ray with defocus blur (for beauty pass)
						ray r_beauty = get_ray(i, j);
						pixel_color += ray_color(r_beauty, world, this->max_depth, env);
						//ray without defocus blur(for render passes) 
						if (s < aux_sample) {
							ray r_aux = get_sharp_ray(i, j);
							hit_record rec;

							//calculate auxiliary data for the same sample
							if (world.hit(r_aux, interval(0.001, infinity), rec)) {
								//albedo 
								pixel_albedo += rec.mat->get_albedo(rec);
								//normals
								vec3 n = unit_vector(rec.normal);
								//transition on camera space(view space)
								double nx = dot(n, u);
								double ny = dot(n, v);
								double nz = dot(n, w);
								//mapping to range[0,1]
								pixel_normal += color(
									(nx + 1.0) * 0.5,
									(ny + 1.0) * 0.5,
									(nz + 1.0) * 0.5
								);
								//z-depth
								double depth_val = rec.t / z_depth_max_dist;
								double z_depth = 1.0 - std::clamp(depth_val, 0.0, 1.0);
								pixel_zdepth += color(z_depth, z_depth, z_depth);
							}
							else {
								//backgrounds
								pixel_albedo += color(0.0, 0.0, 0.0);
								pixel_normal += color(0.5, 0.5, 1.0);
								pixel_zdepth += color(0.0, 0.0, 0.0);
							}
						}
					}

					//average all the buffers
					int idx = j * image_width + i;
					framebuffer[idx] = pixel_color * pixel_samples_scale; //average by all the samples
					//average all the passes by declared aux_sample
					albedo_buffer[idx] = pixel_albedo * aux_scale;
					normal_buffer[idx] = pixel_normal * aux_scale;
					z_depth_buffer[idx] = pixel_zdepth * aux_scale;
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

	void apply_denoising(int width, int height, std::vector<color>& framebuffer,
		std::vector<color>& albedo_buffer, std::vector<color>& normal_buffer) {

		//device initialization with error handling
		oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::Default);
		try {
			device.commit();
		}
		catch (...) {
			//if default (GPU) not working, force CPU
			device = oidn::newDevice(oidn::DeviceType::CPU);
			device.commit();
		}

		//OIDN requires float type data, create buffors for all 3 layers 
		std::vector<float> f_color(width * height * 3);
		std::vector<float> f_albedo(width * height * 3);
		std::vector<float> f_normal(width * height * 3);

		//helper function for clearing values
		auto clean_val = [](double v) {
			if (std::isnan(v) || std::isinf(v)) {
				return 0.0f;
			}
			return static_cast<float>(v);
			};

		for (size_t i = 0; i < framebuffer.size(); ++i) {
			f_color[i * 3 + 0] = static_cast<float>(framebuffer[i].x());
			f_color[i * 3 + 1] = static_cast<float>(framebuffer[i].y());
			f_color[i * 3 + 2] = static_cast<float>(framebuffer[i].z());

			// UŻYJ SWOICH PRAWDZIWYCH, UŚREDNIONYCH DANYCH
			f_albedo[i * 3 + 0] = clean_val(albedo_buffer[i].x());
			f_albedo[i * 3 + 1] = clean_val(albedo_buffer[i].y());
			f_albedo[i * 3 + 2] = clean_val(albedo_buffer[i].z());

			f_normal[i * 3 + 0] = clean_val(normal_buffer[i].x());
			f_normal[i * 3 + 1] = clean_val(normal_buffer[i].y());
			f_normal[i * 3 + 2] = clean_val(normal_buffer[i].z());
		}

		//OIDNBuffer for full GPU/CPU compatibility
		size_t bufferSize = width * height * 3 * sizeof(float);
		oidn::BufferRef colorBuf = device.newBuffer(bufferSize);
		oidn::BufferRef albedoBuf = device.newBuffer(bufferSize);
		oidn::BufferRef normalBuf = device.newBuffer(bufferSize);

		colorBuf.write(0, bufferSize, f_color.data());
		albedoBuf.write(0, bufferSize, f_albedo.data());
		normalBuf.write(0, bufferSize, f_normal.data());

		//configurate filter RT(Ray Tracing)
		oidn::FilterRef filter = device.newFilter("RT");

		//filter
		std::cerr << "Applying filters..." << std::endl;

		//connect all 3 inputs
		filter.setImage("color", colorBuf, oidn::Format::Float3, width, height);
		filter.setImage("albedo", albedoBuf, oidn::Format::Float3, width, height);
		filter.setImage("normal", normalBuf, oidn::Format::Float3, width, height);
		//output filter(save result back to f_color)
		filter.setImage("output", colorBuf, oidn::Format::Float3, width, height);

		filter.set("hdr", true); //for neons and sun
		filter.set("cleanAux", true); //pass info to AI - auxiliary buffers are noise-free
		filter.commit();

		oidn::DeviceType type = device.get<oidn::DeviceType>("type");

		std::cout << "OIDN is running on: ";
		switch (type) {
			case oidn::DeviceType::CPU:   std::cout << "CPU (Procesor)" << std::endl; break;
			case oidn::DeviceType::CUDA:  std::cout << "GPU (NVIDIA CUDA)" << std::endl; break;
			case oidn::DeviceType::SYCL:  std::cout << "GPU (Intel/AMD SYCL)" << std::endl; break;
			case oidn::DeviceType::HIP:   std::cout << "GPU (AMD HIP)" << std::endl; break;
			case oidn::DeviceType::Metal: std::cout << "GPU (Apple Silicon)" << std::endl; break;
			default:                      std::cout << "Unknown Device" << std::endl; break;
		}

		//AI execution
		std::cerr << "Applying HQ AI Denoising..." << std::endl;
		filter.execute();

		const char* err;
		if (device.getError(err) != oidn::Error::None) {
			std::cerr << "OIDN Error during execution: " << err << std::endl;
		}

		colorBuf.read(0, bufferSize, f_color.data());
		std::cerr << "Denoising finished." << std::endl;

		//copying the results back to the framebuffer (from float to double)
		for (size_t i = 0; i < framebuffer.size(); ++i) {
			framebuffer[i] = color(f_color[i * 3 + 0], f_color[i * 3 + 1], f_color[i * 3 + 2]);
			albedo_buffer[i] = color(f_albedo[i * 3 + 0], f_albedo[i * 3 + 1], f_albedo[i * 3 + 2]);
			normal_buffer[i] = color(f_normal[i * 3 + 0], f_normal[i * 3 + 1], f_normal[i * 3 + 2]);
		}
	}

	void process_framebuffer_to_image(
		const std::vector<color>& buffer, 
		const std::string& filename, 
		const post_processor& pp,
		bool is_data_pass = false
	) {
		//conversion framebuffer → RGB
		std::vector<unsigned char> image_data(image_width * image_height * 3);

		//tone mapping and RGB conversion
		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; i++) {
				//get raw color
				color pix_color = buffer[j * image_width + i];

				if (!is_data_pass) {
					//calculate normalized u,v (0.0 - 1.0 range)
					double u = double(i) / (image_width - 1);
					double v = double(j) / (image_height - 1);
					//ACES, saturation, contrast, vignette, gamma correction
					pix_color = pp.process(pix_color, u, v);
				} else {
					pix_color = color(
						std::clamp(pix_color.x(), 0.0, 1.0),
						std::clamp(pix_color.y(), 0.0, 1.0),
						std::clamp(pix_color.z(), 0.0, 1.0));
				}

				int idx = (j * image_width + i) * 3;
				image_data[idx + 0] = static_cast<unsigned char>(255.999 * pix_color.x());
				image_data[idx + 1] = static_cast<unsigned char>(255.999 * pix_color.y());
				image_data[idx + 2] = static_cast<unsigned char>(255.999 * pix_color.z());
			}
		}

		//save to .png
		stbi_write_png(filename.c_str(), image_width, image_height, 3, image_data.data(), image_width * 3);
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

	ray get_sharp_ray(int i, int j) const {
		//transition Antialiasing (jitter)
		auto px = -0.5 + random_double();
		auto py = -0.5 + random_double();
		auto pixel_sample = pixel00_loc + ((i + px) * pixel_delta_u) + ((j + py) * pixel_delta_v);

		//ray Origin here is always camera center
		auto ray_origin = center;
		auto ray_direction = pixel_sample - ray_origin;

		return ray(ray_origin, ray_direction);
	}

	ray get_center_ray(int i, int j) const {
		//hit the center of the pixel(no randomness)
		auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
		auto ray_direction = pixel_center - center;
		return ray(center, ray_direction);
	}

	//returns the vector to a random point in the [-0.5, -0.5]-[0.5, 0.5] unit square
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

