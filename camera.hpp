#include "color.hpp"
#include "hittable.hpp"
#include "material.hpp"
#include "ray.hpp"
#include "vec3.hpp"
#include "stb_image_write.h"
#include "environment.hpp"
#include "color_processing.hpp"
#include "bloom.hpp"
#include <OpenImageDenoise/oidn.hpp>

#include <iostream>
#include <limits>
#include <algorithm>
#include <thread>
#include <functional>
#include <iomanip>
#include <chrono>
#include <vector>
#include <filesystem>

class camera {
public:
	//image settings
	double aspect_ratio = 1.0;  //ratio of image width over height
	int image_width = 400;      //rendered image width in pixel count
	int image_height = 225; //rendered image height
	int samples_per_pixel = 30; //count of random smaples for each pixel
	int current_samples_count = 0; // Licznik aktualnie obliczonych próbek
	int max_depth = 10;         //max recursion depth
	double sky_intesity = 1.0;    //intensity multiplier for sky color default 1.0

	//camera settings
	double vfov = 30; //vertical view angle (field of view)
	point3 lookfrom = point3(10, 1.5, 0); //point where camera is looking from
	point3 lookat = point3(0, 0, 0); //point where camera i s looking at
	vec3 vup = vec3(0, 1, 0); //camera-relative "up" direction

	//defocus blur
	double defocus_angle = 0.5; //variation angle of rays through each pixel
	double focus_dist = 10; //distance from camera lookfrom point to plane of perfect focus

	//denoiser flag
	bool use_denoiser = false;

	//volumetric fog
	bool use_fog = false;
	float fog_density = 0.005f;
	float fog_color[3] = { 0.5f, 0.7f, 1.0f };

	//render passes
	bool use_albedo_buffer = false;
	bool use_normal_buffer = false;
	bool use_z_depth_buffer = false;
	bool use_reflection = false;
	bool use_refraction = false;

	//hdr maps
	std::vector<std::string> hdr_files;

	std::vector<color> render_accumulator; //raw image (no filters applied)

	//create all the necessary buffers
	std::vector<color> albedo_buffer;
	std::vector<color> normal_buffer;
	std::vector<color> z_depth_buffer;
	std::vector<color> reflection_buffer;
	std::vector<color> refraction_buffer;

	std::vector<color> final_framebuffer; //image after post-processing (filters applied)
	std::atomic<int> lines_rendered{ 0 }; //atomic counter for rendered lines

	std::string get_default_hdr_path() const {
		if (!hdr_files.empty()) {
			return HDR_DIR + hdr_files[0];
		}
		return "";
	}

	void refresh_hdr_list() {
		hdr_files.clear();

		try {
			if (!std::filesystem::exists(HDR_DIR)) {
				std::filesystem::create_directories(HDR_DIR);
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(HDR_DIR)) {
				auto ext = entry.path().extension().string();
				if (ext == ".hdr" || ext == ".exr") {
					hdr_files.push_back(entry.path().filename().string());
				}
			}
		}
		catch (...) {}
	}

	void update_post_processing(const post_processor& post, int w, int h) {
		//copy HDR datas before calculating
		if (render_accumulator.empty()) {
			return;
		}
		final_framebuffer = render_accumulator;

		size_t total = final_framebuffer.size();

		//check buffer size 
		if (total != static_cast<size_t>(w) * h) {
			return;
		}
		//Wyliczenie aktualnej ekspozycj
		double current_ev;
		if (post.use_auto_exposure) {
			// Analizujemy jasność skopiowanego bufora
			image_statistics stats = post.analyze_framebuffer(final_framebuffer);
			current_ev = post.apply_auto_exposure(stats);
		}
		else {
			current_ev = post.exposure; //get value in manual mode from slider
		}

		// NAJPIERW nakładamy ekspozycję na bazowy obraz
		for (size_t i = 0; i < total; ++i) {
			final_framebuffer[i] *= current_ev;
		}

		// 1. Bloom & Exposure (Używamy parametrów 'w' i 'h')
		if (post.use_bloom) {
			std::vector<color> bloom_overlay(total, color(0.0, 0.0, 0.0));
			bloom_filter bloom(post.bloom_threshold, post.bloom_intensity, post.bloom_radius);

			bloom.generate_bloom_overlay(final_framebuffer, bloom_overlay, w, h, 1.0f);

			for (size_t i = 0; i < total; ++i) {
				final_framebuffer[i] += bloom_overlay[i];
			}
		}
		else {
			for (size_t i = 0; i < total; ++i) final_framebuffer[i] *= current_ev;
		}

		// 2. Sharpening (Używamy parametrów 'w' i 'h')
		if (post.use_sharpening) {
			post.apply_sharpening(final_framebuffer, w, h, post.sharpen_amount);
		}

		// 3. Efekty końcowe (Color Balance, ACES, Gamma)
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				int idx = j * w + i;

				// Ostateczny bezpiecznik przed crashem
				if (idx >= (int)total) break;

				float u = (w > 1) ? float(i) / (w - 1) : 0.5f;
				float v = (h > 1) ? float(j) / (h - 1) : 0.5f;

				final_framebuffer[idx] = post.process(final_framebuffer[idx], u, v);
			}
		}
	}

	void reset_accumulator() {
		size_t required_size = static_cast<size_t>(image_width) * image_height;

		//auxiliary function for safe buffer preparation
		auto prepare_buffer = [&](std::vector<color>& buf) {
			if (buf.size() != required_size) {
				buf.resize(required_size, color(0.0, 0.0, 0.0));
			}
			else {
				std::fill(buf.begin(), buf.end(), color(0.0, 0.0, 0.0));
			}
			};

		prepare_buffer(render_accumulator);
		prepare_buffer(albedo_buffer);
		prepare_buffer(normal_buffer);
		prepare_buffer(z_depth_buffer);
		prepare_buffer(reflection_buffer);
		prepare_buffer(refraction_buffer);

		// 2. Zerujemy licznik próbkowania (aby zacząć od 1. próbki)
		current_samples_count = 0;
		// 3. Opcjonalnie zerujemy postęp linii
		lines_rendered = 0;
	}

	//render
	void render(const hittable& world, const EnvironmentSettings& env, const post_processor& post, std::atomic<bool>& render_flag) {
		// - 1. INITIALIZE - 
		initialize();

		int num_threads = std::thread::hardware_concurrency();
		std::cerr << "Render threading started with " << num_threads << " threads.\n";

		// - 2. MULTITHREADING  -
		//transfer reference to is_rendering so threads can check if they should stop working
		execute_render_threads(world, env, render_accumulator,
			albedo_buffer, normal_buffer, z_depth_buffer,
			reflection_buffer, refraction_buffer, post.z_depth_max_dist,
			render_flag);


		//if rendering was cancelled, skip post-processing steps 
		if (!render_flag.load()) {
			return;
		}

		std::cerr << "Render completed. Total samples: " << samples_per_pixel << "\n";

		// - 3. AUTO-EXPOSURE -
		if (post.use_auto_exposure) {
			image_statistics stats = post.analyze_framebuffer(render_accumulator);

			double calculated_ev = post.apply_auto_exposure(stats);
			post.exposure = static_cast<float>(calculated_ev);

			std::cerr << "\n[Auto-Exposure] Average Luminance: " << stats.average_luminance << "\n";
			std::cerr << "[Auto-Exposure] New Exposure Value: " << calculated_ev << "\n";
		}

		// - 4. AI DENOISING AND POST-DENOISE SHARPENING -
		if (use_denoiser) {
			//denoise all the necessary passes (linear hdr)
			// 
			//beauty pass
			apply_denoising(image_width, image_height, render_accumulator, albedo_buffer, normal_buffer);

			//reflection pass
			if (use_reflection) {
				apply_denoising(image_width, image_height, reflection_buffer, albedo_buffer, normal_buffer);
				if (post.use_sharpening) {
					post.apply_sharpening(reflection_buffer, image_width, image_height, post.sharpen_amount);
				}
			}
			//refraction pass
			if (use_refraction) {
				apply_denoising(image_width, image_height, refraction_buffer, albedo_buffer, normal_buffer);
				if (post.use_sharpening) {
					post.apply_sharpening(refraction_buffer, image_width, image_height, post.sharpen_amount);
				}
			}
		}

		// - 5. COMPOSE FINAL FRAMEBUFFER WITH POST-PROCESSING -
		std::cerr << "Render finished. Finalizing buffers...\n";
	}

private:
	double pixel_samples_scale = 0.0; //color scale factor for a sum of pixel samples
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
		//ensure that it's at least 1 unit for ratio
		if (image_width < 1) {
			image_width = 1;
		}
		if (image_height < 1) {
			image_height = 1;
		}
		//update aspect ratio 
		aspect_ratio = double(image_width) / image_height;

		pixel_samples_scale = 1.0 / samples_per_pixel;
		center = lookfrom; //center of the camera source

		//determine viewport dimensions
		auto theta = degrees_to_radians(vfov);
		auto h = std::tan(theta / 2);

		auto viewport_height = 2 * h * focus_dist;
		auto viewport_width = viewport_height * aspect_ratio;

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

		refresh_hdr_list();
	}

	void execute_render_threads(
		const hittable& world,
		const EnvironmentSettings& env,
		std::vector<color>& framebuffer,
		std::vector<color>& albedo_buffer,
		std::vector<color>& normal_buffer,
		std::vector<color>& z_depth_buffer,
		std::vector<color>& reflection_buffer,
		std::vector<color>& refraction_buffer,
		double z_depth_max_dist,
		std::atomic<bool>& render_flag) {

		//reset atomic counter for progress bar
		this->lines_rendered = 0;
		//local atomic counter for progress bar in this function
		std::atomic<int> lines_done = 0;

		//number of threads = number of cores
		int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::thread> threads;

		//lamba function for rendering a block of rows
		auto render_rows = [&](int start_y, int end_y) {
			int local_lines_done = 0; //local thread counter

			const int aux_sample = std::clamp(samples_per_pixel / 8, 64, 1024); //for albedo, normals, zdepth
			const int light_pass_sample = samples_per_pixel; //for reflection/refraction

			const double aux_scale = 1.0 / aux_sample;
			const double light_scale = 1.0 / light_pass_sample;

			for (int j = start_y; j < end_y; ++j) {
				//check if rendering should stop
				if (!render_flag.load()) {
					return;
				}

				for (int i = 0; i < image_width; ++i) {
					color pixel_color(0.0, 0.0, 0.0);
					color pixel_albedo(0.0, 0.0, 0.0);
					color pixel_normal(0.0, 0.0, 0.0);
					color pixel_zdepth(0.0, 0.0, 0.0);
					color pixel_reflection(0.0, 0.0, 0.0);
					color pixel_refraction(0.0, 0.0, 0.0);

					//sampling loop for each pixel 
					for (int s = 0; s < samples_per_pixel; s++) {
						ray r = get_ray(i, j);
						hit_record rec;

						//only one collision test for the main ray
						if (world.hit(r, interval(0.001, infinity), rec)) {
							//beauty pass
							pixel_color += ray_color_from_hit(r, rec, world, max_depth, env);

							//get datas (render passes: Albedo, Normals, Z-Depth) from the first hit
							if (s < aux_sample) {
								// - albedo 
								pixel_albedo += rec.mat->get_albedo(rec);
								// - normals
								vec3 n = unit_vector(rec.normal);
								//transition on camera space(view space)
								double nx = dot(n, u);
								double ny = dot(n, v);
								double nz = dot(n, w);
								//mapping to range[0,1]
								pixel_normal += color(
									(nx + 1.0) * 0.5,
									(ny + 1.0) * 0.5,
									(nz + 1.0) * 0.5);

								// - zDepth
								double z_depth = 1.0 - std::clamp(rec.t / z_depth_max_dist, 0.0, 1.0);
								pixel_zdepth += color(z_depth, z_depth, z_depth);
							}

							//reflection and refraction
							//use 'rec' from the first hit
							ray scattered;
							color attenuation;
							if (rec.mat->scatter(r, rec, attenuation, scattered)) {
								//check what the ray hits
								color scattered_color = ray_color(scattered, world, this->max_depth - 1, env);

								//limit maximum luma for reflection/refraction to avoid fireflies
								double luma = 0.2126 * scattered_color.length();
								double max_luma = 2.0; //maximum luma threshold
								if (luma > max_luma) {
									scattered_color *= (max_luma / luma);
								}
								//divide into buffers depending on material type
								//scattered ray has almost the same direction as the perfect reflection:
								vec3 reflected_dir = reflect(unit_vector(r.direction()), unit_vector(rec.normal));
								bool is_specular = dot(unit_vector(scattered.direction()), reflected_dir) > 0.9;

								if (is_specular) {
									pixel_reflection += attenuation * scattered_color;
								}
								else if (dot(scattered.direction(), rec.normal) < 0) {
									//if not mirror check if glass 
									pixel_refraction += attenuation * scattered_color;
								}
							}
						}
						else {
							//background for Beauty Pass
							pixel_color += get_background_color(r, env);
							if (s < aux_sample) {
								//backgrounds for passes (albedo, normals, zdepth)
								pixel_albedo += color(0.0, 0.0, 0.0);
								pixel_normal += color(0.5, 0.5, 1.0);
								pixel_zdepth += color(0.0, 0.0, 0.0);
							}
							//backgrounds for passes reflection, refraction
							pixel_reflection += color(0.0, 0.0, 0.0);
							pixel_refraction += color(0.0, 0.0, 0.0);
						}
					}

					//average all the buffers
					int idx = j * image_width + i;
					framebuffer[idx] = pixel_color * light_scale; //average by all the samples
					//average all the passes by declared aux_sample
					albedo_buffer[idx] = pixel_albedo * aux_scale;
					normal_buffer[idx] = pixel_normal * aux_scale;
					z_depth_buffer[idx] = pixel_zdepth * aux_scale;
					//for those light_scale
					reflection_buffer[idx] = pixel_reflection * light_scale;
					refraction_buffer[idx] = pixel_refraction * light_scale;
				}
				//increase the local counter for progress bar
				local_lines_done++;
				//every 10 lines update progress bar
				if (local_lines_done % 10 == 0 || j == end_y - 1) {
					//fetch_add for atomic safety 
					this->lines_rendered.fetch_add(local_lines_done);
					local_lines_done = 0; //reset locally
				}
			}
			};

		//split the work between threads
		int rows_per_thread = image_height / num_threads;
		int extra = image_height % num_threads;
		int start = 0;
		for (int t = 0; t < num_threads; ++t) {
			int end = start + rows_per_thread + (t < extra ? 1 : 0);
			threads.emplace_back(render_rows, start, end);
			start = end;
		}

		//join threads - finished
		for (auto& th : threads) {
			if (th.joinable()) {
				th.join();
			}
		}

		//final 100% while loop finished
		if (render_flag.load()) {
			this->lines_rendered = image_height; //force 100% for GUI
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
		size_t bufferSize = static_cast<size_t>(width) * height * 3 * sizeof(float);
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
		case oidn::DeviceType::CPU: {
			std::cout << "CPU (Procesor)" << std::endl;
			break;
		}
		case oidn::DeviceType::CUDA: {
			std::cout << "GPU (NVIDIA CUDA)" << std::endl;
			break;
		}
		case oidn::DeviceType::SYCL: {
			std::cout << "GPU (Intel/AMD SYCL)" << std::endl;
			break;
		}
		case oidn::DeviceType::HIP: {
			std::cout << "GPU (AMD HIP)" << std::endl;
			break;
		}
		case oidn::DeviceType::Metal: {
			std::cout << "GPU (Apple Silicon)" << std::endl;
			break;
		}
		default: {
			std::cout << "Unknown Device" << std::endl;
			break;
		}
		}

		//AI execution
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

			//albedo_buffer[i] = color(f_albedo[i * 3 + 0], f_albedo[i * 3 + 1], f_albedo[i * 3 + 2]);
			//normal_buffer[i] = color(f_normal[i * 3 + 0], f_normal[i * 3 + 1], f_normal[i * 3 + 2]);
		}
	}

	void process_framebuffer_to_image(
		const std::vector<color>& buffer,
		const std::string& filename,
		const post_processor& pp,
		bool is_data_pass = false,
		bool apply_gamma = true) {
		//conversion framebuffer → RGB
		std::vector<unsigned char> image_data(image_width * image_height * 3);

		//tone mapping and RGB conversion
		for (int j = 0; j < image_height; j++) {
			for (int i = 0; i < image_width; i++) {
				//get raw color
				color pix_color = buffer[static_cast<size_t>(j) * image_width + i];

				if (!is_data_pass) {
					//calculate normalized u,v (0.0 - 1.0 range)
					float u = static_cast<float>(i) / (image_width - 1);
					float v = static_cast<float>(j) / (image_height - 1);
					//ACES, saturation, contrast, vignette, gamma correction
					pix_color = pp.process(pix_color, u, v);
				}
				else {
					pix_color = color(
						std::clamp(pix_color.x(), 0.0, 1.0),
						std::clamp(pix_color.y(), 0.0, 1.0),
						std::clamp(pix_color.z(), 0.0, 1.0));

					if (apply_gamma) {
						pix_color = linear_to_gamma(pix_color);
					}
				}

				size_t idx = (static_cast<size_t>(j) * image_width + i) * 3;
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

	//returns a random point in the camera defocus disk
	point3 defocus_disk_sample() const {
		auto p = random_in_unit_disk();
		return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}

	color get_background_color(const ray& r, const EnvironmentSettings& env) const {
		vec3 unit_dir = unit_vector(r.direction());

		//solid color background
		if (env.mode == EnvironmentSettings::SOLID_COLOR) {
			return env.background_color * env.intensity;
		}

		//HDR map background
		if (env.mode == EnvironmentSettings::HDR_MAP) {
			if (!env.hdr_texture) {
				return color(0.0, 0.0, 0.0);
			}
			vec3 d = unit_dir;

			//yaw HDR rotation (Y-axis) 
			double cos_y = cos(env.hdri_rotation);
			double sin_y = sin(env.hdri_rotation);
			double x1 = cos_y * d.x() + sin_y * d.z();
			double z1 = -sin_y * d.x() + cos_y * d.z();
			d = vec3(x1, d.y(), z1);

			//pitch/tilt (X-axis) - up/down tilt
			double cos_p = cos(env.hdri_tilt);
			double sin_p = sin(env.hdri_tilt);
			double y2 = cos_p * d.y() - sin_p * d.z();
			double z2 = sin_p * d.y() + cos_p * d.z();
			d = vec3(d.x(), y2, z2);

			//roll (Z-axis)  
			double cos_r = cos(env.hdri_roll);
			double sin_r = sin(env.hdri_roll);
			double x3 = cos_r * d.x() - sin_r * d.y();
			double y3 = sin_r * d.x() + cos_r * d.y();
			d = vec3(x3, y3, d.z());


			//uv mapping
			auto phi = atan2(d.z(), d.x()) + pi;
			auto theta = acos(std::clamp(d.y(), -1.0, 1.0));

			return env.hdr_texture->value(phi / (2 * pi), theta / pi, point3(0.0, 0.0, 0.0)) * env.intensity;
		}
		//physcial sun model
		//
		// normalize sun direction locally 
		vec3 sun_dir = unit_vector(env.sun_direction);
		//day and night parameters
		double sun_height = sun_dir.y();
		double adjusted_height = sun_height - 0.05;
		//sun height 0.0 -> exposure 1.0, 
		//sun height -0.15 -> exposure 0.0
		double sky_exposure = std::clamp(adjusted_height * 8.0 + 1.4, 0.0, 1.0);
		double day_factor = std::clamp(adjusted_height * 10.0 + 1.1, 0.0, 1.0);

		double sunset_intensity = std::clamp(1.0 - std::abs(adjusted_height + 0.05) * 30.0, 0.0, 1.0);
		double sunset_factor = (adjusted_height > -0.1) ? sunset_intensity : 0.0;
		//tone down sunset below horizon
		if (sun_height < 0) {
			sunset_factor *= (sun_height * 10.0 + 1.0);
		}
		sunset_factor = std::clamp(sunset_factor, 0.0, 1.0);

		//sky colors
		color zenit_color = color(0.01, 0.03, 0.1) * (1.0 - day_factor) + color(0.2, 0.5, 1.0) * day_factor;
		color horizon_color = color(0.05, 0.02, 0.01) * (1.0 - day_factor) + color(0.6, 0.8, 1.0) * day_factor;
		//sunset horizon
		horizon_color = horizon_color * (1.0 - sunset_factor) + color(1.0, 0.35, 0.1) * sunset_factor;
		//sky gradient
		auto a = unit_dir.y();
		color sky_color;

		if (a > 0.0) {
			sky_color = (1.0 - a) * horizon_color + a * zenit_color;
		} else {
			// Dla dołu (pod horyzontem) dajemy ciemniejszy kolor horyzontu
			sky_color = horizon_color * 0.1;
		}
		color final_color = sky_color * (env.intensity * 1.5) * sky_exposure;

		//sun disc(physical)
		double sun_focus = dot(unit_dir, sun_dir);
		//sun_size parameter in UI 0.1(small) - 2.0(big)
		// Próg tarczy słońca (używamy Twojego sun_size)
		// Zakładamy, że w UI sun_size to np. 1.0 (małe) do 10.0 (duże)
		double sun_threshold = 1.0 - (env.sun_size * 0.001);

		if (sun_focus > sun_threshold && adjusted_height > -0.1) {
			color s_color = env.sun_color * (1.0 - sunset_factor) + color(1.0, 0.3, 0.1) * sunset_factor;
			double visibility = std::clamp(sun_height * 5.0 + 1.0, 0.0, 1.0);
			//antyaliasing sun edges
			double alpha = smoothstep(sun_threshold, sun_threshold + 0.0002, sun_focus);

			final_color += s_color * env.sun_intensity * visibility * alpha;
		}
		return final_color;
	}

	//get ray and check the hit 
	color ray_color(const ray& r, const hittable& world, int depth, const EnvironmentSettings& env) const {
		color accumulated_light(0.0, 0.0, 0.0);
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

				//early termination if the ray is very weak after 10 bounces
				if (i > 10 && accumulated_attenuation.length() < 0.00001) {
					break;
				}
			}
			else {
				//material absorbed the ray, no more light is gathered
				break;
			}

			//optimization "russian roulette" (for rays bouncing optimazation e.g. after 10 hits)
			if (i > 10) {
				//chance of survival based on the strongest color channel (luminance)
				double p = std::max({ accumulated_attenuation.x(), accumulated_attenuation.y(), accumulated_attenuation.z() });

				//prevent division by zero and too small values
				p = std::clamp(p, 0.05, 0.95);
				//if the ray is very weak, we give it a chance to survive (e.g., 50% or p)
				if (random_double() > p) {
					break; //ray terminated
				}
				// Boost the energy to account for the survival probability
				accumulated_attenuation /= p;
			}
		}
		return accumulated_light;
	}

	//optimazed ray_color (skip first collision test)
	color ray_color_from_hit(const ray& r, const hit_record& first_rec, const hittable& world, int depth, const EnvironmentSettings& env) const {
		color accumulated_light = first_rec.mat->emitted(first_rec.u, first_rec.v, first_rec.p);
		color accumulated_attenuation(1.0, 1.0, 1.0);

		ray scattered;
		color attenuation;

		// Obsługujemy pierwsze rozproszenie
		if (first_rec.mat->scatter(r, first_rec, attenuation, scattered)) {
			accumulated_attenuation *= attenuation;
			// Reszta odbić leci już normalnie w pętli (od i=1 do depth)
			return accumulated_light + accumulated_attenuation * ray_color(scattered, world, depth - 1, env);
		}

		return accumulated_light;
	}
};

