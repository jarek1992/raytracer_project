#include "libs/glad/include/glad/glad.h"
#include "SDL3/SDL.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/backends/imgui_impl_sdl3.h"
#include "libs/imgui/backends/imgui_impl_opengl3.h"

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
#include <thread>
#include <atomic> //for safe communication between threads

GLuint create_texture_from_buffer(const std::vector<color>& buffer, int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	//convert the color buffer(double/float) to float format for OpenGL
	std::vector<float> float_data;
	float_data.reserve(buffer.size() * 3);
	for (const auto& c : buffer) {
		float_data.push_back(static_cast<float>(c.x()));
		float_data.push_back(static_cast<float>(c.y()));
		float_data.push_back(static_cast<float>(c.z()));
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, float_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;

}

int main(int argc, char* argv[]) {
	// - 1. LOADING MATERIALS FROM THE LIBRARY -
	MaterialLibrary mat_lib;
	load_materials(mat_lib); //from scene_management.hpp
	// - 2. LOADING THE GEOMETRY -
	hittable_list world = build_geometry(mat_lib); //from scene_management.hpp
	// - 3. BVH ACCELERATION STRUCTURE -
	hittable_list bvh_world;
	bvh_world.add(make_shared<bvh_node>(world));
	// - 4. CREATE ENVIRONMENT -
	EnvironmentSettings env;
	// - 5. CREATE CAMERA  -
	camera cam;
	// - 6. POST-PROCESSING -
	post_processor my_post;

	// - WINDOW AND OPENGL INITIALIZATION FOR IMGUI DISPLAY OF THE RENDERED IMAGE -
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Ray_Tracer", 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

	// - INITIALIZE IMGUI -
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 330");

	bool running = true;
	std::atomic<bool> is_rendering{ false }; //flag to indicate if rendering is in progress
	GLuint rendered_texture = 0; //global texture ID for rendered image

	//lambda function to restart a new render when settings change (interactive preview window)
	auto start_new_render = [&]() {
		// 1. Wymuś stop obecnego renderu
		is_rendering = false;

		// 2. Bardzo ważne: Poczekaj chwilę, aż wątek zauważy zmianę flagi 
		// lub upewnij się, że Twoja funkcja cam.render() sprawdza is_rendering w pętli

		// 3. Zaktualizuj wysokość na podstawie nowych proporcji
		cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
		if (cam.image_height < 1) {
			cam.image_height = 1;
		}

		cam.lines_rendered = 0;
		is_rendering = true;

		std::thread render_thread([&is_rendering, &cam, bvh_world, env, my_post]() {
			cam.render(bvh_world, env, my_post, is_rendering);
			is_rendering = false;
			});
		render_thread.detach();
		};

	// - MAIN LOOP -
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
		}
		//start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		bool needs_update = false; //flag to indicate if any setting changed
		// - WINDOW 1: CONTROL PANEL -
		ImGui::Begin("Engine Control Panel");
		bool should_restart = false; //flag to indicate if render should be restarted

		if (ImGui::BeginTabBar("Tabs")) {
			//camera settings tab
			if (ImGui::BeginTabItem("Camera & Quality")) {
				//image resolution and aspect ratio
				ImGui::SeparatorText("Resolution & Aspect");
				if (ImGui::InputInt("Image Width", &cam.image_width)) {
					if (cam.image_width < 100) {
						cam.image_width = 100;
					}
					should_restart = true; //always restart while changing resolution
				}
				//slider for aspect ratio
				static float aspect = 1.777f; // 16/9
				if (ImGui::SliderFloat("Aspect Ratio", &aspect, 0.5f, 2.5f, "%.3f")) {
					cam.aspect_ratio = (double)aspect;
					should_restart = true;
				}
				//quick buttons for some standard aspect ratios
				if (ImGui::Button("16:9")) {
					aspect = 1.777f; cam.aspect_ratio = 16.0 / 9.0;
					should_restart = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("4:3")) {
					aspect = 1.333f; cam.aspect_ratio = 4.0 / 3.0;
					should_restart = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("1:1")) {
					aspect = 1.000f; cam.aspect_ratio = 1.0;
					should_restart = true;
				}

				ImGui::SeparatorText("Position & Orientation");
				//look from
				static double lookfrom_buf[3] = { cam.lookfrom.x(), cam.lookfrom.y(), cam.lookfrom.z() };
				if (ImGui::InputScalarN("Look From", ImGuiDataType_Double, lookfrom_buf, 3)) {
					cam.lookfrom = point3(lookfrom_buf[0], lookfrom_buf[1], lookfrom_buf[2]);
					should_restart = true;
				}
				//look at
				static double lookat_buf[3] = {
					cam.lookat.x(), cam.lookat.y(), cam.lookat.z()
				};
				if (ImGui::InputScalarN("Look At", ImGuiDataType_Double, lookat_buf, 3)) {
					cam.lookat = point3(lookat_buf[0], lookat_buf[1], lookat_buf[2]);
					should_restart = true;
				}
				//up vector
				ImGui::Text("Up Vector");
				//create a temporary buffer for InputScalarN to handle the data
				static double vup_buf[3];
				vup_buf[0] = cam.vup.x();
				vup_buf[1] = cam.vup.y();
				vup_buf[2] = cam.vup.z();

				ImGui::PushID("UpVectorFields");
				if (ImGui::InputScalarN("##vup_input", ImGuiDataType_Double, vup_buf, 3)) {
					cam.vup = vec3(vup_buf[0], vup_buf[1], vup_buf[2]);
					should_restart = true;
				}
				ImGui::PopID();
				//reset and normalize
				if (ImGui::Button("Reset Up (0,1,0)")) {
					cam.vup = vec3(0, 1, 0);
					should_restart = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Normalize Up")) {
					cam.vup = unit_vector(cam.vup);
					should_restart = true;
				}
				ImGui::SeparatorText("Optics");
				//fov
				float vfov_f = static_cast<float>(cam.vfov);
				if (ImGui::SliderFloat("FOV", &vfov_f, 1.0f, 120.0f)) {
					cam.vfov = static_cast<double>(vfov_f);
					should_restart = true;
				}
				//aperture (defocus_angle)
				float aperture_f = static_cast<float>(cam.defocus_angle);
				if (ImGui::SliderFloat("Aperture", &aperture_f, 0.0f, 5.0f)) {
					cam.defocus_angle = static_cast<double>(aperture_f);
					should_restart = true;
				}
				//focus distance
				float focus_f = static_cast<float>(cam.focus_dist);
				if (ImGui::SliderFloat("Focus Dist", &focus_f, 0.1f, 50.0f)) {
					cam.focus_dist = static_cast<double>(focus_f);
					should_restart = true;
				}

				ImGui::SeparatorText("Quality");
				if (ImGui::InputInt("Samples per Pixel", &cam.samples_per_pixel)) {
					if (cam.samples_per_pixel < 1) cam.samples_per_pixel = 1;
					should_restart = true;
				}
				if (ImGui::SliderInt("Max Depth", &cam.max_depth, 1, 100)) {
					should_restart = true;
				}
				ImGui::SeparatorText("Render Passes");
				ImGui::Checkbox("Denoise", &cam.use_denoiser);
				ImGui::Checkbox("Albedo", &cam.use_albedo_buffer);
				ImGui::Checkbox("Normals", &cam.use_normal_buffer);
				ImGui::Checkbox("Z-Depth", &cam.use_z_depth_buffer);
				if (cam.use_z_depth_buffer) {
					ImGui::Indent();
					ImGui::SliderFloat("Max Distance", &my_post.z_depth_max_dist, 0.1f, 50.0f);
					ImGui::Unindent();
				}
				ImGui::Checkbox("Reflections (Mirrors)", &cam.use_reflection);
				ImGui::Checkbox("Refractions (Glass)", &cam.use_refraction);
				ImGui::EndTabItem();

				//restart render only if any setting changed and render is pending
				if (should_restart && is_rendering.load()) {
					//signal the render thread to stop 
					is_rendering = false;
					//wait a short moment to ensure the render thread has noticed the change 
					std::this_thread::sleep_for(std::chrono::milliseconds(30));
					//always calculate before starting a new render
					cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
					if (cam.image_height < 1) {
						cam.image_height = 1;
					}
					//reset progress and start new render
					cam.lines_rendered = 0;
					is_rendering = true; //set true so the new thread can start

					std::thread render_thread([&is_rendering, &cam, bvh_world, env, my_post]() {
						cam.render(bvh_world, env, my_post, is_rendering);
						//set false only when render actually finished naturally 
						if (is_rendering.load() && cam.lines_rendered >= cam.image_height) {
							is_rendering = false;
						}
					});
					render_thread.detach();
				}
			}
			//environment settings tab
			if (ImGui::BeginTabItem("Environment")) {
				ImGui::SeparatorText("Sky Mode Selection");
				//switching buttons for environment mode (use enum from EnvironmentSetting)
				int current_mode = (int)env.mode;

				if (ImGui::RadioButton("HDR Map", current_mode == EnvironmentSettings::HDR_MAP)) {
					env.mode = EnvironmentSettings::HDR_MAP;
					should_restart = true;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Physical Sun", current_mode == EnvironmentSettings::PHYSICAL_SUN)) {
					env.mode = EnvironmentSettings::PHYSICAL_SUN;
					should_restart = true;
				}
				ImGui::Separator();
				float intensity_f = static_cast<float>(env.intensity);
				if (ImGui::SliderFloat("Global Intensity", &intensity_f, 0.0f, 5.0f)) {
					env.intensity = static_cast<double>(intensity_f);
					should_restart = true;
				}
				//HDRI mode settings
				if (env.mode == EnvironmentSettings::HDR_MAP) {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HDR Map Settings");
					//desplay the name of the loaded texture (informational)
					ImGui::Text("Active Texture: assets/sunny_rose_garden_2k.hdr");
					//rotation around Y-axis and tilt around X-axis
					static float rot_deg = -45.0f;
					static float tilt_deg = -4.0f;

					if (ImGui::Button("Reset Orientation")) {
						rot_deg = 0.0f;
						tilt_deg = 0.0f; //reset parameters to default
						env.hdri_rotation = 0.0;
						env.hdri_tilt = 0.0;
						should_restart = true;
					}
					if (ImGui::SliderFloat("Rotation (Y-axis)", &rot_deg, -180.0f, 180.0f)) {
						env.hdri_rotation = degrees_to_radians(rot_deg);
						should_restart = true;
					}
					if (ImGui::SliderFloat("Tilt (X-axis)", &tilt_deg, -90.0f, 90.0f)) {
						env.hdri_tilt = degrees_to_radians(tilt_deg);
						should_restart = true;
					}
				}
				//physical Sun mode settings
				if (env.mode == EnvironmentSettings::PHYSICAL_SUN) {
					ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Physical Sun & Sky");
					//sun color
					float sun_col[3] = { (float)env.sun_color.x(), (float)env.sun_color.y(), (float)env.sun_color.z() };
					if (ImGui::ColorEdit3("Sun Color", sun_col)) {
						env.sun_color = color(sun_col[0], sun_col[1], sun_col[2]);
						should_restart = true;
					}
					//sun intensity
					float sun_int_f = static_cast<float>(env.sun_intensity);
					if (ImGui::SliderFloat("Sun Intensity", &sun_int_f, 0.0f, 20.0f)) {
						env.sun_intensity = static_cast<double>(sun_int_f);
						should_restart = true;
					}
					//sun size
					float sun_size_f = static_cast<float>(env.sun_size);
					if (ImGui::SliderFloat("Sun Size", &sun_size_f, 100.0f, 10000.0f)) {
						env.sun_size = static_cast<double>(sun_size_f);
						should_restart = true;
					}
					//sun direction
					ImGui::Text("Sun Direction");
					bool sun_dir_changed = false;
					sun_dir_changed |= ImGui::InputDouble("X##sun", &env.sun_direction[0]); ImGui::SameLine();
					sun_dir_changed |= ImGui::InputDouble("Y##sun", &env.sun_direction[1]); ImGui::SameLine();
					sun_dir_changed |= ImGui::InputDouble("Z##sun", &env.sun_direction[2]);

					if (sun_dir_changed) {
						should_restart = true;
					}
					//button to normalize the sun direction vector
					if (ImGui::Button("Normalize Direction")) {
						env.sun_direction = unit_vector(env.sun_direction);
						should_restart = true;
					}
				}
				ImGui::EndTabItem();
			}
			//post-processing tab
			if (ImGui::BeginTabItem("Post-Process")) {

				ImGui::SeparatorText("Debug Views");
				const char* debug_items[] = { "NONE", "RED", "GREEN", "BLUE", "LUMINANCE" };
				int current_mode = (int)my_post.current_debug_mode;
				if (ImGui::Combo("Debug Mode", &current_mode, debug_items, IM_ARRAYSIZE(debug_items))) {
					my_post.current_debug_mode = (debug_mode)current_mode;
					needs_update = true; //setting changed
				}

				ImGui::Spacing();
				ImGui::Separator();

				needs_update |= ImGui::Checkbox("ACES Tone Mapping", &my_post.use_aces_tone_mapping);
				needs_update |= ImGui::Checkbox("Auto Exposure", &my_post.use_auto_exposure);
				needs_update |= ImGui::SliderFloat("Exposure", &my_post.exposure, 0.0f, 5.0f);

				ImGui::SeparatorText("Color Grade");
				needs_update |= ImGui::SliderFloat("Contrast", &my_post.contrast, 0.5f, 2.0f);
				needs_update |= ImGui::SliderFloat("Saturation", &my_post.saturation, 0.0f, 2.0f);
				needs_update |= ImGui::ColorEdit3("Color Balance", (float*)&my_post.color_balance);
				needs_update |= ImGui::SliderFloat("Hue Shift", &my_post.hue_shift, -180.0f, 180.0f);

				ImGui::SeparatorText("Effects");
				ImGui::SliderFloat("Vignette", &my_post.vignette_intensity, 0.0f, 1.0f);

				if (ImGui::CollapsingHeader("Bloom Settings")) {
					needs_update |= ImGui::Checkbox("Enable Bloom", &my_post.use_bloom);
					needs_update |= ImGui::SliderFloat("Threshold", &my_post.bloom_threshold, 0.0f, 5.0f);
					needs_update |= ImGui::SliderFloat("Intensity##Bloom", &my_post.bloom_intensity, 0.0f, 5.0f);
					needs_update |= ImGui::SliderInt("Radius", &my_post.bloom_radius, 1, 15);
				}
				//sharpening
				if (ImGui::CollapsingHeader("Sharpening")) {
					needs_update |= ImGui::Checkbox("Enable Sharpen", &my_post.use_sharpening);
					float sharp_f = static_cast<float>(my_post.sharpen_amount);
					if (ImGui::SliderFloat("Amount", &sharp_f, 0.0f, 1.0f)) {
						my_post.sharpen_amount = static_cast<double>(sharp_f);
						needs_update = true;
					}
				}
				//update the rendered image preview only if settings changed and not currently rendering
				if (needs_update) {
					cam.update_post_processing(my_post);
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::Separator();
		//render button and progress bar
		if (is_rendering) {
			float progress = (float)cam.lines_rendered / (float)cam.image_height;
			ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rendering: %.1f%%", progress * 100.0f);
		}

		if (ImGui::Button("Render Frame") && !is_rendering) {
			// 1. Zawsze aktualizuj wysokość przed startem! [cite: 2026-01-18]
			cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
			if (cam.image_height < 1) {
				cam.image_height = 1;
			}
			cam.lines_rendered = 0; //reset progress
			is_rendering = true; //block the button to prevent starting multiple renders at once
			//create a detached thread to run the render function
			std::thread render_thread([&is_rendering, &cam, bvh_world, env, my_post]() {
				cam.render(bvh_world, env, my_post, is_rendering);
				//set false only when render actually finished naturally
				if (is_rendering.load() && cam.lines_rendered >= cam.image_height) {
					is_rendering = false;
				}
			});
			render_thread.detach();
		}

		//add a small status indicator next to the button
		if (is_rendering) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rendering in progress...");
		}
		ImGui::End();

		// - WINDOW 2: RENDER PREVIEW -
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); //remove padding for full image display
		ImGuiWindowFlags preview_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse; //NoScrollbar i NoScrollWithMouse
		if (ImGui::Begin("Render Preview", nullptr, preview_flags)) {
			static bool last_rendering_state = false;
			bool rendering_active = is_rendering.load();
			bool just_finished = (last_rendering_state == true && rendering_active == false);
			bool has_data = !cam.render_accumulator.empty();

			if (just_finished || (has_data && rendered_texture == 0) || rendering_active || (needs_update && !rendering_active)) {
				if (rendered_texture != 0) {
					glDeleteTextures(1, &rendered_texture);
				}
				//update the texture from the render accumulator
				if (rendering_active) {
					//copy local buffer to avoid threading issues
					std::vector<color> buffer_copy = cam.render_accumulator;
					//get actual buffer size during rendering (to handle partial renders correctly)
					size_t actual_size = buffer_copy.size();

					if (actual_size > 0 && (actual_size % cam.image_width == 0)) {
						int current_w = cam.image_width;
						int current_h = (int)(actual_size / current_w);

						std::vector<color> processed_preview(actual_size);
						for (size_t i = 0; i < actual_size; ++i) {
							size_t x = i % current_w;
							size_t y = i / current_w;

							//use a copy (buffer-copy) to avoid error out of range
							processed_preview[i] = my_post.process(buffer_copy[i], (float)x / current_w, (float)y / current_h);
						}
						rendered_texture = create_texture_from_buffer(processed_preview, current_w, current_h);
					}
				} else {
					//final image - apply post-processing to the full render accumulator
					if (!cam.final_framebuffer.empty()) {
						rendered_texture = create_texture_from_buffer(cam.final_framebuffer, cam.image_width, cam.image_height);
					}
				}
				needs_update = false;
			}

			//display the texture with automatic scaling (fit the window size)
			if (rendered_texture != 0) {
				ImVec2 avail_size = ImGui::GetContentRegionAvail();
				//add automatic scaling to fit the window
				float image_aspect = (float)cam.image_width / (float)cam.image_height;
				float window_aspect = avail_size.x / avail_size.y;
				float display_w, display_h;

				//fit the image to the window while preserving aspect ratio
				if (image_aspect > window_aspect) {
					//image is wider than window -> fit to width
					display_w = avail_size.x;
					display_h = avail_size.x / image_aspect;
				} else {
					//image is higher than window -> fit to height
					display_h = avail_size.y;
					display_w = avail_size.y * image_aspect;
				}

				//calculate offsets for centering the image in the window 
				float offset_x = (avail_size.x - display_w) * 0.5f;
				float offset_y = (avail_size.y - display_h) * 0.5f;

				////move the cursor to center the image
				ImGui::SetCursorPos(ImVec2(offset_x, offset_y));
				//display the image 
				ImGui::Image((ImTextureID)(intptr_t)rendered_texture, ImVec2(display_w, display_h));
			} else {
				ImGui::Text("Ready to render...");
			}
			last_rendering_state = rendering_active;
		}
		ImGui::End();
		ImGui::PopStyleVar(); //restore window padding

		//rendering GUI
		ImGui::Render();
		glViewport(0, 0, 1280, 720);
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	//CLEAN UP IMGUI AND SDL
	//
	//ImGui shutdown
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	//OpenGL and SDL shutdown
	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
