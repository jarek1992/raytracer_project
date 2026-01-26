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

		// - WINDOW 1: CONTROL PANEL -
		ImGui::Begin("Engine Control Panel");
		if (ImGui::BeginTabBar("Tabs")) {
			//camera settings tab
			if (ImGui::BeginTabItem("Camera & Quality")) {
				//image resolution and aspect ratio
				ImGui::SeparatorText("Resolution & Aspect");
				if (ImGui::InputInt("Image Width", &cam.image_width)) {
					if (cam.image_width < 100) cam.image_width = 100; //prevent too small resolutions
				}
				//slider for aspect ratio
				static float aspect = 1.777f; // 16/9
				if (ImGui::SliderFloat("Aspect Ratio", &aspect, 0.5f, 2.5f, "%.3f")) {
					cam.aspect_ratio = (double)aspect;
				}
				//quick buttons for some standard aspect ratios
				if (ImGui::Button("16:9")) { aspect = 1.777f; cam.aspect_ratio = 16.0 / 9.0; }
				ImGui::SameLine();
				if (ImGui::Button("4:3")) { aspect = 1.333f; cam.aspect_ratio = 4.0 / 3.0; }
				ImGui::SameLine();
				if (ImGui::Button("1:1")) { aspect = 1.000f; cam.aspect_ratio = 1.0; }

				ImGui::SeparatorText("Position & Orientation");
				//look from
				static double lookfrom_buf[3] = { cam.lookfrom.x(), cam.lookfrom.y(), cam.lookfrom.z() };
				if (ImGui::InputScalarN("Look From", ImGuiDataType_Double, lookfrom_buf, 3)) {
					cam.lookfrom = point3(lookfrom_buf[0], lookfrom_buf[1], lookfrom_buf[2]);
				}
				//look at
				static double lookat_buf[3] = { 
					cam.lookat.x(), cam.lookat.y(), cam.lookat.z() 
				};
				if (ImGui::InputScalarN("Look At", ImGuiDataType_Double, lookat_buf, 3)) {
					cam.lookat = point3(lookat_buf[0], lookat_buf[1], lookat_buf[2]);
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
				}
				ImGui::PopID();
				//reset and normalize
				if (ImGui::Button("Reset Up (0,1,0)")) {
					cam.vup = vec3(0, 1, 0);
				}
				ImGui::SameLine();
				if (ImGui::Button("Normalize Up")) {
					cam.vup = unit_vector(cam.vup);
				}

				ImGui::SeparatorText("Optics");
				//fov
				float vfov_f = static_cast<float>(cam.vfov);
				if (ImGui::SliderFloat("FOV", &vfov_f, 1.0f, 120.0f)) {
					cam.vfov = static_cast<double>(vfov_f);
				}
				//aperture (defocus_angle)
				float aperture_f = static_cast<float>(cam.defocus_angle);
				if (ImGui::SliderFloat("Aperture", &aperture_f, 0.0f, 5.0f)) {
					cam.defocus_angle = static_cast<double>(aperture_f);
				}
				//focus distance
				float focus_f = static_cast<float>(cam.focus_dist);
				if (ImGui::SliderFloat("Focus Dist", &focus_f, 0.1f, 50.0f)) {
					cam.focus_dist = static_cast<double>(focus_f);
				}

				ImGui::SeparatorText("Quality");
				ImGui::InputInt("Samples per Pixel", &cam.samples_per_pixel);
				ImGui::SliderInt("Max Depth", &cam.max_depth, 1, 100);

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
			}
			//environment settings tab
			if (ImGui::BeginTabItem("Environment")) {

				ImGui::SeparatorText("Sky Mode Selection");

				//switching buttons for environment mode (use enum from EnvironmentSetting)
				int current_mode = (int)env.mode;
				bool mode_changed = false;

				if (ImGui::RadioButton("HDR Map", current_mode == EnvironmentSettings::HDR_MAP)) {
					env.mode = EnvironmentSettings::HDR_MAP;
					mode_changed = true;
				}
				ImGui::SameLine();
				if (ImGui::RadioButton("Physical Sun", current_mode == EnvironmentSettings::PHYSICAL_SUN)) {
					env.mode = EnvironmentSettings::PHYSICAL_SUN;
					mode_changed = true;
				}

				ImGui::Separator();
				float intensity_f = static_cast<float>(env.intensity);
				if (ImGui::SliderFloat("Global Intensity", &intensity_f, 0.0f, 5.0f)) {
					env.intensity = static_cast<double>(intensity_f);
				}

				//HDRI mode settings
				if (env.mode == EnvironmentSettings::HDR_MAP) {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HDR Map Settings");

					// Wyświetlamy nazwę załadowanej tekstury (informacyjnie)
					ImGui::Text("Active Texture: assets/sunny_rose_garden_2k.hdr");

					//rotation around Y-axis and tilt around X-axis
					static float rot_deg = -45.0f;
					static float tilt_deg = -4.0f;

					if (ImGui::SliderFloat("Rotation (Y-axis)", &rot_deg, -180.0f, 180.0f)) {
						env.hdri_rotation = degrees_to_radians(rot_deg);
					}
					if (ImGui::SliderFloat("Tilt (X-axis)", &tilt_deg, -90.0f, 90.0f)) {
						env.hdri_tilt = degrees_to_radians(tilt_deg);
					}
				}

				//physical Sun mode settings
				if (env.mode == EnvironmentSettings::PHYSICAL_SUN) {
					ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Physical Sun & Sky");

					//sun color
					static float sun_col[3] = { (float)env.sun_color.x(), (float)env.sun_color.y(), (float)env.sun_color.z() };
					if (ImGui::ColorEdit3("Sun Color", sun_col)) {
						env.sun_color = color(sun_col[0], sun_col[1], sun_col[2]);
					}
					//sun intensity
					float sun_int_f = static_cast<float>(env.sun_intensity);
					if (ImGui::SliderFloat("Sun Intensity", &sun_int_f, 0.0f, 20.0f)) {
						env.sun_intensity = static_cast<double>(sun_int_f);
					}
					//sun size
					float sun_size_f = static_cast<float>(env.sun_size);
					if (ImGui::SliderFloat("Sun Size", &sun_size_f, 100.0f, 10000.0f)) {
						env.sun_size = static_cast<double>(sun_size_f);
					}

					//sun direction
					ImGui::Text("Sun Direction");
					ImGui::InputDouble("X##sun", &env.sun_direction[0]); ImGui::SameLine();
					ImGui::InputDouble("Y##sun", &env.sun_direction[1]); ImGui::SameLine();
					ImGui::InputDouble("Z##sun", &env.sun_direction[2]);

					//button to normalize the sun direction vector
					if (ImGui::Button("Normalize Direction")) {
						env.sun_direction = unit_vector(env.sun_direction);
					}
				}
				ImGui::EndTabItem();
			}
			//post-processing tab
			if (ImGui::BeginTabItem("Post-Process")) {
				bool needs_update = false; //flag to indicate if any setting changed

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
				if (needs_update && !is_rendering) {
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
			is_rendering = true; //block the button to prevent starting multiple renders at once
			cam.lines_rendered = 0; //reset progress

			//create a detached thread to run the render function
			std::thread render_thread([&is_rendering, &cam, bvh_world, env, my_post]() {
				cam.render(bvh_world, env, my_post);
				is_rendering = false;
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
		if (ImGui::Begin("Render Preview")) {
			static bool last_rendering_state = false;
			bool rendering_active = is_rendering.load();

			//select the appropriate buffer to display based on rendering state 
			const std::vector<color>& current_buffer = (rendering_active) ? cam.render_accumulator : cam.final_framebuffer;

			//check conditions for refreshing the texture from the framebuffer data
			bool just_finished = (last_rendering_state == true && is_rendering == false);
			bool has_data = !current_buffer.empty();

			//update the texture if just finished rendering or if there's new data but no texture yet
			if (just_finished || (has_data && rendered_texture == 0) || rendering_active) {
				if (rendered_texture != 0) {
					glDeleteTextures(1, &rendered_texture); //delete old texture
				}
				rendered_texture = create_texture_from_buffer(current_buffer, cam.image_width, cam.image_height);
			}

			//display the texture with automatic scaling (fit the window size)
			if (rendered_texture != 0) {
				ImVec2 avail_size = ImGui::GetContentRegionAvail();
				//add automatic scaling to fit the window
				float aspect_ratio = (float)cam.image_width / (float)cam.image_height;
				float display_w = avail_size.x;
				float display_h = avail_size.x / aspect_ratio;

				//if height exceeds available height, scale down
				if (display_h > avail_size.y) {
					display_h = avail_size.y;
					display_w = display_h * aspect_ratio;
				}

				ImGui::Image((ImTextureID)(intptr_t)rendered_texture, ImVec2(display_w, display_h));
			} else {
				ImGui::Text("Ready to render. Set your parameters and click 'Render Frame'.");
			}
			last_rendering_state = rendering_active;
		}
		ImGui::End();

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