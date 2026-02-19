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
#include <chrono>

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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, float_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;
}

int main(int argc, char* argv[]) {
	// - 1. LOADING MATERIALS FROM THE LIBRARY -
	MaterialLibrary mat_lib;
	load_materials(mat_lib); //from scene_management.hpp

	// - 2. OBJECT LOADER (.obj)  -
	sceneAssetsLoader assets;

	// - 3. CREATE CAMERA  -
	camera cam;
	cam.refresh_hdr_list(); //scan folder assets/hdr_maps/

	// - 4. LOADING THE GEOMETRY -
	hittable_list world = build_geometry(mat_lib,
		assets,
		cam.use_fog,
		static_cast<double>(cam.fog_density),
		color(cam.fog_color[0], cam.fog_color[1], cam.fog_color[2])); //from scene_management.hpp

	// - 5. BVH ACCELERATION STRUCTURE -
	shared_ptr<hittable> bvh_world = make_shared<bvh_node>(world);

	// - 6. CREATE ENVIRONMENT -
	EnvironmentSettings env;
	env.load_hdr(cam.get_default_hdr_path());
	env.intensity = 1.0;

	// - 7. POST-PROCESSING -
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
	std::chrono::steady_clock::time_point render_start_time; //to measure render duration
	bool is_active_session = false; 
	float last_render_duration = 0.0f;
	bool render_was_cancelled = false;
	float last_progress_percent = 0.0f;
	GLuint rendered_texture = 0; //global texture ID for rendered image
	std::thread render_thread; //thread declaration

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
		bool should_restart = false; //flag to indicate if render should be restarted

		// - WINDOW 1: CONTROL PANEL -
		ImGui::Begin("Engine Control Panel");
		if (ImGui::BeginTabBar("Tabs")) {
			//camera settings tab
			if (ImGui::BeginTabItem("Camera & Quality")) {
				//buffers definition
				static double lookfrom_buf[3] = {
					cam.lookfrom.x(),
					cam.lookfrom.y(),
					cam.lookfrom.z()
				};
				static double lookat_buf[3] = {
					cam.lookat.x(),
					cam.lookat.y(),
					cam.lookat.z()
				};
				static double vup_buf[3] = {
					cam.vup.x(),
					cam.vup.y(),
					cam.vup.z()
				};

				//refresh the buffers
				if (!ImGui::IsAnyItemActive()) {
					lookfrom_buf[0] = cam.lookfrom.x();
					lookfrom_buf[1] = cam.lookfrom.y();
					lookfrom_buf[2] = cam.lookfrom.z();

					lookat_buf[0] = cam.lookat.x();
					lookat_buf[1] = cam.lookat.y();
					lookat_buf[2] = cam.lookat.z();

					vup_buf[0] = cam.vup.x();
					vup_buf[1] = cam.vup.y();
					vup_buf[2] = cam.vup.z();
				}

				//image resolution and aspect ratio
				ImGui::SeparatorText("Resolution & Aspect");
				bool rendering = is_rendering.load();
				//blockage for aspect ratio while rendering
				if (rendering) {
					ImGui::BeginDisabled();
				}
				if (ImGui::InputInt("Image Width", &cam.image_width)) {
					if (cam.image_width < 100) {
						cam.image_width = 100;
					}
				}
				//flag for customized image ratio
				static bool custom_ratio = false;
				if (ImGui::Checkbox("Custom Resolution (Unlock Height)", &custom_ratio)) {
					cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
				}
				if (!custom_ratio) {
					//slider for aspect ratio
					static float aspect = 1.777f; // 16/9
					if (ImGui::SliderFloat("Aspect Ratio", &aspect, 0.5f, 2.5f, "%.3f")) {
						cam.aspect_ratio = (double)aspect;
					}
					//quick buttons for some standard aspect ratios
					if (ImGui::Button("16:9")) {
						aspect = 1.777f;
						cam.aspect_ratio = 16.0 / 9.0;
					}
					ImGui::SameLine();
					if (ImGui::Button("4:3")) {
						aspect = 1.333f;
						cam.aspect_ratio = 4.0 / 3.0;
					}
					ImGui::SameLine();
					if (ImGui::Button("1:1")) {
						aspect = 1.000f;
						cam.aspect_ratio = 1.0;
					}

					//calculate image height automatically
					cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
					ImGui::Text("Locked Height: %d", cam.image_height);
				} else {
					//custom mode: height is unlocked
					if (ImGui::InputInt("Image Height", &cam.image_height)) {
						if (cam.image_height < 100) {
							cam.image_height = 100;
						}

						//update aspect ratio
						cam.aspect_ratio = (double)cam.image_width / cam.image_height;
					}
					ImGui::TextDisabled("Current Ratio: %.3f", cam.aspect_ratio);
				}
				//unlock blockage for aspect ratio section
				if (rendering) {
					ImGui::EndDisabled();
				}

				ImGui::SeparatorText("Position & Orientation");
				//look from
				if (ImGui::InputScalarN("Look From", ImGuiDataType_Double, lookfrom_buf, 3)) {
					cam.lookfrom = point3(lookfrom_buf[0], lookfrom_buf[1], lookfrom_buf[2]);
					should_restart = true;
				}
				//look at
				if (ImGui::InputScalarN("Look At", ImGuiDataType_Double, lookat_buf, 3)) {
					cam.lookat = point3(lookat_buf[0], lookat_buf[1], lookat_buf[2]);
					should_restart = true;
				}
				//up vector
				ImGui::Text("Up Vector");
				//create a temporary buffer for InputScalarN to handle the data
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
				//dropdown passes
				ImGui::Text("Active View:");
				//reference to static array of pass names in camera class
				if (ImGui::BeginCombo("##SelectPass", camera::pass_names[static_cast<int>(cam.current_display_pass)])) {
					for (int n = 0; n < 7; n++) {
						render_pass p = static_cast<render_pass>(n);

						//display only if checkbox for the pass is active (except RGB always active)
						bool is_enabled = true;
						if (p == render_pass::DENOISE) {
							is_enabled = cam.use_denoiser;
						}
						else if (p == render_pass::ALBEDO) {
							is_enabled = cam.use_albedo_buffer;
						}
						else if (p == render_pass::NORMALS) {
							is_enabled = cam.use_normal_buffer;
						}
						else if (p == render_pass::REFLECTIONS) {
							is_enabled = cam.use_reflection;
						}
						else if (p == render_pass::REFRACTIONS) {
							is_enabled = cam.use_refraction;
						}
						else if (p == render_pass::Z_DEPTH) {
							is_enabled = cam.use_z_depth_buffer;
						}
						if (!is_enabled && p != render_pass::RGB) {
							continue;
						}
						const bool is_selected = (cam.current_display_pass == p);
						if (ImGui::Selectable(camera::pass_names[n], is_selected)) {
							cam.current_display_pass = p;
							my_post.needs_update = true;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				//passes checkboxes for activation (except RGB always active)
				ImGui::Text("Compute Passes:");

				//block checkboxes during rendering
				bool is_rendering_now = is_rendering.load();
				ImGui::BeginDisabled(is_rendering_now);

				ImGui::Checkbox("Denoise", &cam.use_denoiser);
				ImGui::Checkbox("Albedo", &cam.use_albedo_buffer);
				ImGui::Checkbox("Normals", &cam.use_normal_buffer);
				ImGui::Checkbox("Reflections(Mirrors)", &cam.use_reflection);
				ImGui::Checkbox("Refractions(Glass)", &cam.use_refraction);
				ImGui::Checkbox("Z-Depth", &cam.use_z_depth_buffer);
				if (cam.use_z_depth_buffer) {
					ImGui::Indent();
					if (ImGui::SliderFloat("Max Distance", &my_post.z_depth_max_dist, 0.1f, 50.0f)) {
						my_post.needs_update = true;
					}
					ImGui::Unindent();
				}

				ImGui::EndDisabled();
				ImGui::EndTabItem();
			}
			//environment settings tab
			if (ImGui::BeginTabItem("Environment")) {
				//buffer definition 
				static double sun_dir_buf[3] = {
					env.sun_direction.x(),
					env.sun_direction.y(),
					env.sun_direction.z()
				};

				static float rot_deg = 0.0f;
				static float tilt_deg = 0.0f;
				static float roll_deg = 0.0f;

				static float sun_col[3] = {
					(float)env.sun_color.x(),
					(float)env.sun_color.y(),
					(float)env.sun_color.z()
				};

				static float bg_col[3] = {
					(float)env.background_color.x(),
					(float)env.background_color.y(),
					(float)env.background_color.z()
				};


				//desychronization and crash prevention
				if (!ImGui::IsAnyItemActive()) {
					sun_dir_buf[0] = env.sun_direction.x();
					sun_dir_buf[1] = env.sun_direction.y();
					sun_dir_buf[2] = env.sun_direction.z();

					sun_col[0] = (float)env.sun_color.x();
					sun_col[1] = (float)env.sun_color.y();
					sun_col[2] = (float)env.sun_color.z();

					bg_col[0] = (float)env.background_color.x();
					bg_col[1] = (float)env.background_color.y();
					bg_col[2] = (float)env.background_color.z();

				}

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
				ImGui::SameLine();
				if (ImGui::RadioButton("Solid Color", current_mode == EnvironmentSettings::SOLID_COLOR)) {
					env.mode = EnvironmentSettings::SOLID_COLOR;
					should_restart = true;
				}

				ImGui::Separator();

				float intensity_f = static_cast<float>(env.intensity);
				if (ImGui::SliderFloat("Global Intensity", &intensity_f, 0.0f, 5.0f)) {
					env.intensity = static_cast<double>(intensity_f);
					should_restart = true;
				}

				//environmental fog
				if (ImGui::CollapsingHeader("Environmental Fog")) {
					if (ImGui::Checkbox("Enable Fog", &cam.use_fog)) {
						should_restart = true;
					}
					if (cam.use_fog) {
						if (ImGui::SliderFloat("Density", &cam.fog_density, 0.0001f, 0.05f, "%.4f")) {
							should_restart = true;
						}
						if (ImGui::ColorEdit3("Fog Color", cam.fog_color)) {
							should_restart = true;
						}
					}
				}

				//HDRI mode settings
				if (env.mode == EnvironmentSettings::HDR_MAP) {
					if (!ImGui::IsAnyItemActive()) {
						rot_deg = static_cast<float>(radians_to_degrees(env.hdri_rotation));
						tilt_deg = static_cast<float>(radians_to_degrees(env.hdri_tilt));
						roll_deg = static_cast<float>(radians_to_degrees(env.hdri_roll));
					}
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "HDR Map Settings");
					//dynamic hdr loading
					ImGui::Text("Active HDR map: %s", env.current_hdr_name.c_str());

					if (ImGui::BeginCombo("Select HDR", env.current_hdr_name.c_str())) {
						for (int n = 0; n < cam.hdr_files.size(); n++) {
							const bool is_selected = (env.current_hdr_name == cam.hdr_files[n]);

							if (ImGui::Selectable(cam.hdr_files[n].c_str(), is_selected)) {

								//loading
								env.load_hdr(HDR_DIR + cam.hdr_files[n]);

								//render reset
								cam.reset_accumulator();
								should_restart = true;

								//rotation parameters reset for a new hdr map
								rot_deg = 0.0f;
								tilt_deg = 0.0f;
								roll_deg = 0.0f;
								env.hdri_rotation = 0.0;
								env.hdri_tilt = 0.0;
								env.hdri_roll = 0.0;
							}
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					if (ImGui::Button("Refresh Folder")) {
						cam.refresh_hdr_list();
					}

					ImGui::Separator();

					if (ImGui::Button("Reset Orientation")) {
						//reset parameters to default
						rot_deg = 0.0f;
						tilt_deg = 0.0f;
						roll_deg = 0.0f;
						env.hdri_rotation = 0.0;
						env.hdri_tilt = 0.0;
						env.hdri_roll = 0.0;
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
					if (ImGui::SliderFloat("Roll (Z-axis)", &roll_deg, -180.0f, 180.0f)) {
						env.hdri_roll = degrees_to_radians(roll_deg);
						should_restart = true;
					}
				}

				//physical Sun mode settings
				if (env.mode == EnvironmentSettings::PHYSICAL_SUN) {
					ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Physical Sun & Sky");
					//sun color
					//checkbox for sun color blockage 
					if (ImGui::Checkbox("Auto Sun Color (Atmospheric)", &env.auto_sun_color)) {
						should_restart = true;
					}
					//visual blockage for ColorEdit
					if (env.auto_sun_color) {
						ImGui::BeginDisabled();
					}
					if (ImGui::ColorEdit3("Sun Color", sun_col)) {
						env.sun_color = color(sun_col[0], sun_col[1], sun_col[2]);
						should_restart = true;
					}
					//end sun color blockage
					if (env.auto_sun_color) {
						ImGui::EndDisabled();
					}
					//sun intensity
					float sun_int_f = static_cast<float>(env.sun_intensity);
					if (ImGui::SliderFloat("Sun Intensity", &sun_int_f, 0.0f, 20.0f)) {
						env.sun_intensity = static_cast<double>(sun_int_f);
						should_restart = true;
					}
					//sun size
					float sun_size_f = static_cast<float>(env.sun_size);
					if (ImGui::SliderFloat("Sun Size", &sun_size_f, 0.0f, 10.0f)) {
						env.sun_size = static_cast<double>(sun_size_f);
						should_restart = true;
					}

					ImGui::Separator();

					//mode switch 
					static bool use_astro = false;
					ImGui::Checkbox("Use daylight System (time/data)", &use_astro);

					if (use_astro) {
						//static variable for converter
						static float sun_time = 12.0f; //0-24 hours
						static int sun_day = 172; //day of the year (1-365, 172 is the summer solstice)
						static float latitude = 52.0f; //latitude (set as default Poland 52)

						bool astro_changed = false;
						astro_changed |= ImGui::SliderFloat("Hour", &sun_time, 0.0f, 24.0f, "%.2f h");
						astro_changed |= ImGui::SliderInt("Day", &sun_day, 1, 365);
						astro_changed |= ImGui::SliderFloat("Latitude", &latitude, -90.0f, 90.0f, "%.1f deg");

						if (astro_changed) {
							//astronomical maths
							double declination = 23.45 * std::sin(degrees_to_radians(360.0 / 365.0 * (sun_day - 81)));
							double hour_angle = (sun_time - 12.0) * 15.0;
							double lat_rad = degrees_to_radians(latitude);
							double dec_rad = degrees_to_radians(declination);
							double ha_rad = degrees_to_radians(hour_angle);

							double sin_el = std::sin(lat_rad) * std::sin(dec_rad) +
								std::cos(lat_rad) * std::cos(dec_rad) * std::cos(ha_rad);
							double elevation = radians_to_degrees(std::asin(std::clamp(sin_el, -1.0, 1.0)));

							double cos_az = (std::sin(dec_rad) - std::sin(lat_rad) * sin_el) /
								(std::cos(lat_rad) * std::cos(std::asin(sin_el)));
							double azimuth = radians_to_degrees(std::acos(std::clamp(cos_az, -1.0, 1.0)));
							if (hour_angle > 0) {
								azimuth = 360.0 - azimuth;
							}
							//update directional vector
							env.sun_direction = direction_from_spherical(elevation, azimuth);

							//automatic sun color
							if (env.auto_sun_color) {
								double s_height = env.sun_direction.y();
								//higher the sun more red less blue
								double warm_factor = std::clamp(1.0 - s_height * 2.0, 0.0, 1.0);
								env.sun_color = color(1.0, 0.9 - warm_factor * 0.6, 0.8 - warm_factor * 0.8);

								//update color buffer 
								sun_col[0] = (float)env.sun_color.x();
								sun_col[1] = (float)env.sun_color.y();
								sun_col[2] = (float)env.sun_color.z();
							}

							//synchronize buffer
							sun_dir_buf[0] = env.sun_direction.x();
							sun_dir_buf[1] = env.sun_direction.y();
							sun_dir_buf[2] = env.sun_direction.z();

							should_restart = true;
						}
					}
					//sun direction
					if (use_astro) {
						ImGui::BeginDisabled();
					}

					ImGui::Text("Sun Direction (Manual)");
					if (ImGui::InputScalarN("##sun_dir_input", ImGuiDataType_Double, sun_dir_buf, 3)) {
						env.sun_direction = vec3(sun_dir_buf[0], sun_dir_buf[1], sun_dir_buf[2]);
						should_restart = true;
					}
					//button to normalize the sun direction vector
					ImGui::SameLine();
					if (ImGui::Button("Normalize")) {
						env.sun_direction = unit_vector(env.sun_direction);
						//update buffer to normalize values 
						sun_dir_buf[0] = env.sun_direction.x();
						sun_dir_buf[1] = env.sun_direction.y();
						sun_dir_buf[2] = env.sun_direction.z();
						should_restart = true;
					}
					if (use_astro) {
						ImGui::EndDisabled();
						ImGui::TextDisabled("(Controlled by Daylight System)");
					}
				}

				//solid background color
				if (env.mode == EnvironmentSettings::SOLID_COLOR) {
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Solid Color Mode");

					if (ImGui::ColorEdit3("Background Color", bg_col)) {
						env.background_color = color(bg_col[0], bg_col[1], bg_col[2]);
						should_restart = true;
					}
				}

				ImGui::EndTabItem();
			}
			//post-processing tab
			if (ImGui::BeginTabItem("Post-Process")) {
				//buffers definition
				static float col_bal[3] = {
					(float)my_post.color_balance.x(),
					(float)my_post.color_balance.y(),
					(float)my_post.color_balance.z()
				};
				//synchronization 
				if (!ImGui::IsItemActive()) {
					col_bal[0] = (float)my_post.color_balance.x();
					col_bal[1] = (float)my_post.color_balance.y();
					col_bal[2] = (float)my_post.color_balance.z();
				}

				ImGui::SeparatorText("Debug Views");

				//auxiliary function
				auto DebugToggle = [&](const char* id, bool& flag, ImVec4 col) {
					int pushed_colors = 0;
					int pushed_vars = 0;

					if (flag) {
						//highligt active debug mode with brighter color and border
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x * 1.2f, col.y * 1.2f, col.z * 1.2f, 1.0f));
						pushed_colors++;
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 1));
						pushed_colors++;
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
						pushed_vars++;
					}
					else {
						ImGui::PushStyleColor(ImGuiCol_Button, col);
						pushed_colors++;
					}

					if (ImGui::Button(id, ImVec2(30, 30))) {
						flag = !flag; //toggle the debug flag
						//if color debug mode is activated, deactivate luminance and vice versa
						if (&flag == &my_post.debug.luminance) {
							if (my_post.debug.luminance) {
								//if L on = R, G, B off
								my_post.debug.red = my_post.debug.green = my_post.debug.blue = false;
							}
							else {
								//if L off = R, G, B on
								my_post.debug.red = my_post.debug.green = my_post.debug.blue = true;
							}
						}
						else {
							//if any of the R G B on = L off
							my_post.debug.luminance = false;
						}
						my_post.needs_update = true;
					}

					ImGui::PopStyleColor(pushed_colors);
					ImGui::PopStyleVar(pushed_vars);
					};

				//buttons row
				if (ImGui::Button("N", ImVec2(30, 30))) {
					my_post.debug.red = true;
					my_post.debug.green = true;
					my_post.debug.blue = true;
					my_post.debug.luminance = false;
					my_post.needs_update = true;
				}

				ImGui::SameLine();
				DebugToggle("R", my_post.debug.red, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
				ImGui::SameLine();
				DebugToggle("G", my_post.debug.green, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
				ImGui::SameLine();
				DebugToggle("B", my_post.debug.blue, ImVec4(0.1f, 0.1f, 0.6f, 1.0f));
				ImGui::SameLine();
				DebugToggle("L", my_post.debug.luminance, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

				//display legend only if LUMINANCE debug mode on
				if (my_post.debug.luminance) {
					ImGui::Spacing();
					ImGui::Text("Luminance False Color Legend:");

					//lambda function to draw small color squares with infos
					auto ColorLabel = [](const char* label, ImVec4 color) {
						ImGui::ColorButton(label, color, ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
						ImGui::SameLine();
						ImGui::Text("%s", label);
						};

					ColorLabel("100%+ (Clipping)", ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
					ColorLabel("95%-100% (Near Clip)", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
					ColorLabel("70%-95% (Highlights)", ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
					ColorLabel("40%-70% (Midtones)", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					ColorLabel("10%-40% (Shadow Detail)", ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
					ColorLabel("2%-10% (Deep Shadows)", ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
					ColorLabel("0%-2% (Black Out)", ImVec4(0.1f, 0.0f, 0.2f, 1.0f));
				}

				ImGui::Spacing();
				ImGui::SeparatorText("Luminance Histogram");

				//normalized histogram plot (256 bins, range 0-1)
				ImGui::PlotHistogram("##HDR_Histogram",
					my_post.last_stats.normalized_histogram, 256,
					0, NULL, 0.0f, 1.0f, ImVec2(-1, 80));

				ImVec2 pos = ImGui::GetItemRectMin();
				ImVec2 size = ImGui::GetItemRectSize();
				ImDrawList* draw_list = ImGui::GetWindowDrawList();

				//target luminance line (orange)
				float target_x = pos.x + (my_post.target_luminance * size.x);
				draw_list->AddLine(
					ImVec2(target_x, pos.y), 
					ImVec2(target_x, pos.y + size.y), 
					IM_COL32(255, 165, 0, 255), 2.0f
				);

				//current avg luma (green)
				float avg_x = pos.x + (std::clamp((float)my_post.last_stats.average_luminance, 0.0f, 1.0f) * size.x);
				draw_list->AddLine(
					ImVec2(avg_x, pos.y),
					ImVec2(avg_x, pos.y + size.y),
					IM_COL32(0, 255, 0, 255), 1.0f
				);

				//display crucial luminance stats 
				ImGui::Text("Avg Luma: %.4f", my_post.last_stats.average_luminance);
				ImGui::SameLine();
				ImGui::Text("| Max Luma: %.2f", my_post.last_stats.max_luminance);

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Logarithmic scale histogram showing HDR distribution.");
				}

				my_post.needs_update |= ImGui::Checkbox("ACES Tone Mapping", &my_post.use_aces_tone_mapping);

				my_post.needs_update |= ImGui::Checkbox("Auto Exposure", &my_post.use_auto_exposure);
				if (my_post.use_auto_exposure) {
					ImGui::Indent();

					//taget luminance slider (Twoje 0.12f)
					if (ImGui::SliderFloat("Target Luminance", &my_post.target_luminance, 0.01f, 0.50f, "%.2f")) {
						// Natychmiastowa aktualizacja ekspozycji po zmianie celu
						my_post.exposure = (float)my_post.apply_auto_exposure(my_post.last_stats);
						my_post.needs_update = true;
					}

					//slider in "stops"(EV) units - (+2)lighten or (-2)darken
					if (ImGui::SliderFloat("Exposure Compensation", &my_post.exposure_compensation_stops, -5.0f, 5.0f, "%.1f EV")) {
						my_post.exposure = (float)my_post.apply_auto_exposure(my_post.last_stats);
						my_post.needs_update = true;
					}
					ImGui::Unindent();
				} else {
					my_post.needs_update |= ImGui::SliderFloat("Manual Exposure", &my_post.exposure, 0.01f, 5.0f, "%.2f");
				}

				ImGui::SeparatorText("Color Grade");
				my_post.needs_update |= ImGui::SliderFloat("Contrast", &my_post.contrast, 0.5f, 2.0f);
				my_post.needs_update |= ImGui::SliderFloat("Saturation", &my_post.saturation, 0.0f, 2.0f);

				if (ImGui::ColorEdit3("Color Balance", col_bal)) {
					my_post.color_balance = vec3(col_bal[0], col_bal[1], col_bal[2]);
					my_post.needs_update = true;
				}
				my_post.needs_update |= ImGui::SliderFloat("Hue Shift", &my_post.hue_shift, -180.0f, 180.0f);

				ImGui::SeparatorText("Effects");
				if (ImGui::SliderFloat("Vignette", &my_post.vignette_intensity, 0.0f, 1.0f)) {
					my_post.needs_update = true;
				}

				if (ImGui::CollapsingHeader("Bloom Settings")) {
					bool changed = false;
					changed |= ImGui::Checkbox("Enable Bloom", &my_post.use_bloom);

					if (my_post.use_bloom) {
						ImGui::Indent();
						changed |= ImGui::SliderFloat("Threshold", &my_post.bloom_threshold, 0.0f, 5.0f, "%.2f");
						changed |= ImGui::SliderFloat("Intensity", &my_post.bloom_intensity, 0.0f, 2.0f, "%.2f");
						changed |= ImGui::SliderInt("Radius", &my_post.bloom_radius, 1, 20);
						ImGui::Unindent();
					}
					if (changed) {
						my_post.needs_update = true; //flag to recalculate post-processing
					}
				}
				//sharpening
				if (ImGui::CollapsingHeader("Sharpening")) {
					if (ImGui::Checkbox("Enable Sharpen", &my_post.use_sharpening)) {
						my_post.needs_update = true;
					}

					float sharp_f = static_cast<float>(my_post.sharpen_amount);
					if (ImGui::SliderFloat("Amount", &sharp_f, 0.0f, 1.0f)) {
						my_post.sharpen_amount = static_cast<double>(sharp_f);
						my_post.needs_update = true;
					}
				}
				ImGui::EndTabItem();
			}
			//export to files tab
			if (ImGui::BeginTabItem("Export")) {
				ImGui::SeparatorText("Export Output");

				//block buttons at the beginning and during rendering
				bool disable_export = !is_rendering && (cam.lines_rendered > 0);

				if (!disable_export) {
					ImGui::BeginDisabled();
				}

				//option to save single pass
				if (ImGui::Button("Save Current Pass", ImVec2(-1, 0))) {
					std::string pass_name = camera::pass_names[static_cast<int>(cam.current_display_pass)];
					std::string name = "render_" + pass_name + ".png";
					cam.save_render_pass(cam.current_display_pass, name, my_post);
				}

				//option to save all passes at once
				if (ImGui::Button("Save All Passes", ImVec2(-1, 0))) {
					for (int n = 0; n < 7; n++) {
						render_pass p = static_cast<render_pass>(n);

						bool is_enabled = (p == render_pass::RGB); //RGB always enabled
						if (p == render_pass::DENOISE) {
							is_enabled = cam.use_denoiser;
						} else if (p == render_pass::ALBEDO) {
							is_enabled = cam.use_albedo_buffer;
						} else if (p == render_pass::NORMALS) {
							is_enabled = cam.use_normal_buffer;
						} else if (p == render_pass::REFLECTIONS) {
							is_enabled = cam.use_reflection;
						} else if (p == render_pass::REFRACTIONS) {
							is_enabled = cam.use_refraction;
						} else if (p == render_pass::Z_DEPTH) {
							is_enabled = cam.use_z_depth_buffer;
						}

						if (is_enabled) {
							std::string pass_name = camera::pass_names[static_cast<int>(p)];
							std::string name = "render_" + pass_name + ".png";
							cam.save_render_pass(p, name, my_post);
						}
					}
				}

				if (!disable_export) {
					ImGui::EndDisabled();

					//logs 
					if (is_rendering) {
						ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rendering... wait to enable export.");
					} else {
						ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No render data.Click Render to start.");
					} 
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		//render control & progress
		ImGui::Separator();
		if (is_active_session) {
			//progress bar
			float progress = (float)cam.lines_rendered / (float)cam.image_height;
			ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

			if (is_rendering) {
				auto current_now = std::chrono::steady_clock::now();
				std::chrono::duration<float> live_elapsed = current_now - render_start_time;

				float estimated_total = (progress > 0.01f) ? (live_elapsed.count() / progress) : 0.0f;

				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Rendering: %.1f%%", progress * 100.0f);
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "(%.2f seconds)", live_elapsed.count());

				if (estimated_total > 0.0f) {
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Est. Total: %.2fs | Remaining: %.2fs",
						estimated_total, estimated_total - live_elapsed.count());
				}
			} else if (last_render_duration > 0.0f) {
				if (render_was_cancelled) {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Render Cancelled at %.1f%%", last_progress_percent);
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Elapsed Time: %.2f seconds", last_render_duration);
				} else {
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Render Finished (100%)");
					ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Render Time: %.2f seconds", last_render_duration);
				}
			}

			//stop render button
			const char* button_label = is_rendering ? "Stop Render" : "Clear";
			ImGui::PushStyleColor(ImGuiCol_Button, is_rendering ? ImVec4(0.6f, 0.1f, 0.1f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

			if (ImGui::Button(button_label, ImVec2(-1, 0))) {
				if (is_rendering) {
					// 1. TYLKO zatrzymujemy renderowanie i zapisujemy statystyki
					auto render_end_time = std::chrono::steady_clock::now();
					std::chrono::duration<float> elapsed = render_end_time - render_start_time;
					last_render_duration = elapsed.count();
					last_progress_percent = ((float)cam.lines_rendered / (float)cam.image_height) * 100.0f;
					render_was_cancelled = true;

					is_rendering = false; // To zatrzyma wątek
					if (render_thread.joinable()) {
						render_thread.join();
					}
					// NIE ustawiamy is_active_session = false, żeby komunikat został na ekranie!
				} else {
					// 2. Jeśli render już nie trwa, to drugie kliknięcie zamyka sesję (czyści GUI)
					is_active_session = false;
				}
			}
			ImGui::PopStyleColor();
		} else {
			//start render button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.1f, 1.0f)); //green color
			if (ImGui::Button("Render", ImVec2(-1, 0))) {
				render_start_time = std::chrono::steady_clock::now(); //start the timer for the new render
				last_render_duration = 0.0f; //reset last render duration for display purposes
				render_was_cancelled = false;
				cam.lines_rendered = 0;
				is_active_session = true;
				should_restart = true; //launch central restart system below
			}
			ImGui::PopStyleColor();
		}

		//central restart render system
		if (should_restart && is_active_session) {
			is_rendering = false;

			if (render_thread.joinable()) {
				render_thread.join();
			}

			hittable_list world = build_geometry(
				mat_lib,
				assets,
				cam.use_fog,
				static_cast<double>(cam.fog_density),
				color(cam.fog_color[0], cam.fog_color[1], cam.fog_color[2])
			);

			// 3. Aktualizujemy BVH dla nowej geometrii (w tym mgły)
			bvh_world = make_shared<bvh_node>(world);

			// 4. Resetujemy akumulator i licznik próbek (zgodnie z wcześniejszą naprawą)
			cam.reset_accumulator();

			//prepare datas for a new render
			cam.image_height = static_cast<int>(cam.image_width / cam.aspect_ratio);
			if (cam.image_height < 1) {
				cam.image_height = 1;
			}

			//launch the new thread
			is_rendering = true;
			render_thread = std::thread([&is_rendering, &cam, bvh_world, env, my_post, &last_render_duration, render_start_time]() {
				cam.render(*bvh_world, env, my_post, is_rendering);

				//finalize - only if render finished without interrupts
				if (is_rendering.load() && cam.lines_rendered >= cam.image_height) {
					auto render_end_time = std::chrono::steady_clock::now();
					std::chrono::duration<float> elapsed = render_end_time - render_start_time;
					last_render_duration = elapsed.count();

					is_rendering = false;
				}
			});

			should_restart = false;
		}

		ImGui::End();

		// - WINDOW 2: RENDER PREVIEW -
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); //remove padding for full image display
		ImGuiWindowFlags preview_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse; //NoScrollbar and NoScrollWithMouse
		if (ImGui::Begin("Render Preview", nullptr, preview_flags)) {
			static bool last_rendering_state = false;
			static auto last_preview_update = std::chrono::steady_clock::now();

			bool rendering_active = is_rendering.load();
			//crucial moment when progress bar finished 
			bool just_finished = (last_rendering_state == true && rendering_active == false);

			//check if e.i. 100ms pasted from the last buffer copy
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_preview_update).count();
			bool time_to_update = (elapsed > 150); // 150ms = 10 FPS

			if (just_finished || (rendering_active && time_to_update) || my_post.needs_update) {
				//lock the size(thread-safe mindset)
				int locked_w = cam.image_width;
				int locked_h = cam.image_height;

				//if rendering: get the latest data from the accumulator for preview (thread-safe mindset)
				if (rendering_active || just_finished) {
					cam.final_framebuffer = cam.render_accumulator;
				}

				//get reference to selected buffer from passes dropdown
				const std::vector<color>& buffer_to_show = cam.get_active_buffer();

				//check if we have data to display
				if (!buffer_to_show.empty() && buffer_to_show.size() == (size_t)locked_w * locked_h) {
					//clean old texture from GPU to prevent memory leak
					if (rendered_texture != 0) {
						glDeleteTextures(1, &rendered_texture);
					}
					//if display rgb or denoise passes apply postprocessing
					if (cam.current_display_pass == render_pass::RGB || cam.current_display_pass == render_pass::DENOISE) {
						//analyze buffer for post-processing (e.g. auto-exposure)
						image_statistics stats = my_post.analyze_framebuffer(cam.render_accumulator);
						my_post.last_stats = stats;

						//autoexposure
						if (my_post.use_auto_exposure) {
							my_post.exposure = (float)my_post.apply_auto_exposure(stats);
						}
					}

					//callout post-processing update if needed
					cam.update_post_processing(my_post, locked_w, locked_h);
					//create a new texture
					rendered_texture = create_texture_from_buffer(cam.final_framebuffer, locked_w, locked_h);

					if (rendering_active) {
						last_preview_update = now;
					}
				}
				my_post.needs_update = false;
			}

			//display the texture with automatic scaling (fit the window size)
			if (rendered_texture != 0 && cam.image_height > 0) {
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
				}
				else {
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
			}
			else {
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
	//cleaunp section
	//
	//stop render and wait for thread 
	is_rendering = false;
	is_active_session = false;

	if (render_thread.joinable()) {
		render_thread.join(); // wait till thread ends
	}

	//remove texture from GPU
	if (rendered_texture != 0) {
		glDeleteTextures(1, &rendered_texture);
	}

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