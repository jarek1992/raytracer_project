## 🚀 Zenith | Modern C++20 Path Tracing Engine
A high-performance, physically-based path tracing engine built with C++20. This project features a robust real-time diagnostic UI, sophisticated environmental lighting, and a professional post-processing pipeline. 

<img width="1200" height="675" alt="render_main" src="https://github.com/user-attachments/assets/10faae78-c023-4ff2-b227-0d4816d0408e" />

### 🌟 <b>Key Project Highlights:</b>
- <b>Global Illumination Out-of-the-box:</b> Full Monte Carlo Path Tracing that naturally handles indirect lighting, soft shadows, and color bleeding.
- <b>Physically Based Camera & Optics:</b> Thin-lens simulation providing cinematic <b>Depth of Field (Bokeh)</b> and real-time focus pulling.
- <b>Production-Ready Post-Processing:</b> Complete HDR pipeline featuring <b>ACES Tone Mapping</b>, physically-based Bloom, and Auto-Exposure.
- <b>High-Performance Scalability:</b> <i>O(logN)</i> ray-traversal via custom <b>BVH structures</b> and 100% CPU utilization via <b>OpenMP</b>.
- <b>Interactive Diagnostic Suite:</b> Real-time G-Buffer visualization (Normals, Albedo, Depth) and a live Luminance Histogram.
- <b>Intelligent State Synchronization:</b> Decoupled UI and Render states using a <b>Dirty Flag system</b>, allowing for fluid parameter manipulation with smart accumulator management.

### 🛠 Core Technical Features
<ul>
  <details>
    <summary><b>Engine Specifications</b></summary>
    <ul>
      <p><li><b>Language:</b> <b>C++20</b> (utilizing modern standards: <code>std::clamp</code>, <code>std::shared_ptr</code>, and advanced lambdas).</li>
        <li><b>Rendering Model:</b> Progressive Path Tracing (real-time sample accumulation).</li>
        <li><b>Integration Method:</b> Monte Carlo (stochastic sampling of light paths).
	
```cpp      
//ACES Tone Mapping to map HDR radiance to [0.0, 1.0] display range
inline color apply_aces(color x) {
	const double a = 2.51;
	const double b = 0.03;
	const double c = 2.43;
	const double d = 0.59;
	const double e = 0.14;

	return color(
		std::clamp((x.x() * (a * x.x() + b)) / (x.x() * (c * x.x() + d) + e), 0.0, 1.0),
		// ... applied to all channels
	);
}
```		
  </li>
  <li><b>Hardware Acceleration(CPU):</b> <b>OpenMP</b> (parallelized computation across all available processor threads).</li>
 
  ```cpp
  #pragma omp parallel for schedule(dynamic)
  for (int j = 0; j < image_height; ++j) {
  	//pixel processing logic...
  }
```
  <li><b>Data Structures:</b> <b>BVH(Bounding Volume Hierarchy)</b> – optimizes ray-object intersection tests from <i>O(N)</i> to <i>O(logN)</i>.</li>
  <ul>
    <br>
	  <img width="609" height="263" alt="aabb_bvh_diagram" src="https://github.com/user-attachments/assets/688fd571-8e4a-47a6-af85-1574fe8f0a4d" />
	  <p><i>Spatial bounding volume hierarchy (BVH) reduces the complexity of collision tests from <b>O(N)</b> to <b>O(log N)</b> through recursive division of the scene into containers AABB.</i></p>
  </ul>
  <br>
  <li>
    <b>Memory Management:</b> Dirty Flag System (<code>needs_update</code>, <code>needs_ui_sync</code>) – intelligent buffer reloading triggered only upon parameter changes.

```cpp
//example of smart state synchronization
if (ImGui::SliderFloat("Aperture", &cam.aperture, 0.0f, 0.5f)) {
	my_post.needs_update = true; //trigger post-process recalculation
}
      
if (ImGui::IsItemDeactivatedAfterEdit()) {
	reset_accumulator(); //only reset samples when user finishes interaction
}
```  
  </li>
</ul>
    </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Camera & Optics System</b></summary>
    <br>
    <ul>
        
| **Feature** | **Description** | **Key Parameters** |
| :--- | :--- | :--- |
| Thin Lens Model | *Simulation of a physical lens for realistic bokeh effects* | *Aperture (f-stop), Focus Distance* |
| Dynamic FOV | *Full perspective control for architectural or macro shots* | *Vertical FOV (degrees)* |
| Interactive Navigation | *Smooth 3D space movement and orientation* | *LookAt, LookFrom, Up Vector* |
| Anti-aliasing(AA) | *<b>Sub-pixel jittering</b> eliminates jagged edges by slightly <br>offsetting rays within each pixel footprint, <br>resulting in smooth, film-like edges* | *Stratified Sampling (per pixel)* |


<p><code>IMAGE: 3 images with different focus distances 3.0, 9.0, and 18.0.</code></p>

  </ul>
  </details>
  </ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Environment & Lighting Systems</b></summary>
    <p>The engine provides a versatile lighting suite, allowing users to switch between physical sky simulations, image-based lighting, and studio backgrounds.</p>
    <ul> 
      <p><li><b>HDRI Maps(IBL):</b></li>
      <p>For photorealistic reflections and complex ambient lighting, the engine supports industry-standard High Dynamic Range images.</p>
      <ul>
        <p><code>IMAGE: 2-3 obrazki obok siebie w roznych oswietleniach map HDR</code></p>
        <li><b>Technology:</b><i> Native support for 32-bit .hdr files (IBL) providing a massive range of luminance data.</i></li>
        <li><b>3-Axis Transformation:</b><i> Full spherical orientation control using Yaw, Pitch, and Roll to align the environment with your scene geometry perfectly.</i></li>
        <li><b>Asset Management:</b><i> Integrated file observer allows for dynamic refreshing of the HDRI library. Add new maps to the directory and select them in-app without a restart.</i></li>
        <br>
      </ul>
      <li><b>Astronomical Daylight System:</b></li>
      <p>This module simulates the sun's position and color based on real-world celestial mechanics. It offers two distinct modes of operation:</p>
      <ul>
        <p><code>IMAGE: 2-3 obrazki obok siebie w roznych oswietleniach przy uzycie astronomical sun(6am, 12pm, 8pm)</code></p>
        <li><b>Astronomical Mode:</b><i> Calculates the sun's position automatically using geographical and temporal data.</i></li>
        <ul style="list-style-type: disc; margin-left: 20px;">
          <li><b>Parameters:</b><i> Latitude (coordinates), Day of the Year (1-365), and Time of Day (0-24h).</i></li>
          <li><b>Celestial Math:</b><i> Implementation of solar declination, hour angle, azimuth, and elevation algorithms to ensure pinpoint accuracy.</i></li>
          <br>
        </ul>
        <li><b>Manual Mode:</b><i> Provides direct control over the sun's direction vector for artistic lighting setups.</i></li>
        <li><b>Auto-Sun Color:</b><i>Dynamically adjusts the solar spectrum based on altitude.</i></li>
        <ul style="list-style-type: disc; margin-left: 20px;">
          <li><b>Atmospheric Simulation:</b><i> Implements a simplified Rayleigh Scattering model; as the sun nears the horizon, the increased optical path length through the atmosphere shifts the light toward warmer, reddish wavelengths, while high-altitude sun positions produce a crisp, cooler white.</i></li>
        </ul>
        <br>
      </ul>
      <li><b>Solid Background:</b></li>
      <p>Designed for product photography and clean architectural presentations.</p>
      <ul style="list-style-type: disc; margin-left: 20px;">
        <p><code>IMAGE: 1 obrazek z solid background</code></p>
        <li><b><i>Control:</b> Full RGB spectrum selection via a precision color picker.</i></li>
        <li><b><i>Intensity:</b> Adjustable background radiance, allowing for neutral studio setups that don't overpower the scene's primary light sources.</i></li>
      </ul>
        </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Material Library(PBR)</b></summary>
    <br>
    <ul>
        
| **Material** | **Physical Property** | **Key Features** | **Textures(Albedo)** | **Bump Map** |
| :--- | :--- | :--- | :--- | :--- |
| Lambertian | *Ideal Diffuse* | *Simulates matte surfaces with perfect light scattering.* | *✅ Yes* | *✅ Yes* |
| Metal | *Specular Reflection* | *Includes a <b>Fuzz</b> parameter to control surface roughness/blurriness of reflections.* | *✅ Yes* | *✅ Yes* |
| Dielectric | *Refraction* | *Handles transparent materials like glass or water with <b>IOR</b> (Index of Refraction) and Total Internal Reflection.* | *✅ Color* | *✅ Yes* |
| Emissive | *Light Emission* | *Turns any geometry into a physical light source (<b>Area Light</b>) with adjustable radiance.* | *✅ Color/Power* | *❌ No* |

<p><code>IMAGE: kule obok siebie z roznymi materialami - diffused lambertian, bumped metal, glass(dielectric), emissive.</code></p>

<p><li><b>Technical Highlights & Material System</b></li>
<p>The engine's material system is built on physical principles, ensuring that every interaction between light and geometry behaves as it would in the real world.</p>
<ul> 
  <li><b>Energy Conservation:</b><i> Every material is mathematically constrained to ensure reflected light never exceeds incoming energy, preserving physical consistency and preventing "unrealistic glowing" artifacts.</i></li>
  <li><b>Stochastic Importance Sampling:</b><i> Reflection and refraction directions are calculated using Monte Carlo importance sampling. This enables the simulation of complex optical effects like soft reflections and frosted glass with high efficiency.</i></li>
  <li><b>Ray-Material Interaction:</b><i> Each material implements a unique scattering function. Based on physical constants (like IOR or Fuzz), the engine decides whether a ray is absorbed, reflected, or refracted.</i></li>
</ul>
<p><li><b>Texture & Surface Mapping</b></li>
<ul>
  <p><li><b>Texture-Material Integration:</b><i> The engine supports mapping textures to any geometric primitive. You can blend procedural or image-based textures with any PBR material(e.g., a textured metal or a patterned emissive surface).</i></li>
  <li><b>Bump Mapping(Beta):</b><i> Preliminary support for Bump Mapping is available for basic primitives(cube and sphere), allowing for fine-grained surface detail without increasing polygon count.</i></li>
  <ul style="list-style-type: none;">
    <p><i>Note: Currently, Bump Mapping is not supported for .obj triangle meshes; this is planned for a future update.</i></p>
</ul>
</details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Cinematic Post-Process Pipeline</b></summary>
    <p>Beyond path tracing, the engine includes a high-performance post-processing stack to achieve a production-ready look.</p>
    <p><code> IMAGE: raw image/image with post-production: ACES + BLOOM + EXPOSURE FIXES</code></p>
      <ul>     
        <li><b>ACES Tone Mapping:</b>Implementation of the Academy Color Encoding System to transform High Dynamic Range (HDR) data into cinematic Low Dynamic Range (LDR) output.</li>
        <li><b>Bloom Engine:</b> A physically-inspired glow effect that extracts highlights and bleeds them into surrounding pixels using a configurable threshold and blur radius.</li>
        <li><b>Exposure Control:</b>
      <ul>
        <li><b>Auto-Exposure:</b><i> Note: Dynamically calculates scene luminance to adjust brightness.</li></i>
          <ul>
           <p><code>IMAGE: HISTOGRAM LUMINANCE - mały zrzut ekranu samej konsoli/histogramu obok opisu Auto-Exposure. To pokaże, że "automatyka" opiera się na rzeczywistych danych statystycznych obrazu.</code></p>
          </ul>
        <li><b>EV Compensation:</b><i> Photographic control allowing for ±5.0 stops of brightness adjustment.</li></i>
      </ul>
        </li>
      </ul>
  </details>
</ul>

### 🎬 Scene Configuration & Workflow
<ul>
  The entire scene configuration is centralized in <code>scene_management.hpp</code>. You don't need to recompile the core engine to swap assets or materials.
</ul>
<ul style="list-style-type: none;">
  <details>
    <summary><b></b>Assets Loader</b></summary>
    <p>A dedicated struct <code>sceneAssetsLoader</code> for pre-loading heavy <code>.obj</code> models (like the included <code>teapot.obj</code> or <code>bowl.obj</code>) into memory once and stored as shared pointers to optimize RAM usage. Models can be placed <code>/assets/models/</code></p>

```cpp
struct sceneAssetsLoader {
	shared_ptr<model> teapot;

	sceneAssetsLoader() {
		teapot = make_shared<model>("assets/models/teapot.obj", nullptr, 0.4);
		//add more .obj models here...
	}
};
```
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Material Library & Instancing</b></summary>
    <p>Zenith Engine features a high-performance Material Library system that separates surface properties from geometric data. This allows for massive memory savings through the Flyweight pattern.</p>
    <ul>
      <li><b>Centralized Registry:</b> Define all your materials once in a global library <code>(load_materials)</code>. It supports <b>lambertian</b>, <b>metal</b>, <b>dielectric(glass)</b>, and <b>emissive</b>.</li>
      <li><b>Texture & Bump Mapping:</b> Enhance surface detail using high-resolution texture and bump maps. Pass an <code>image_texture</code> to the material constructor to add tactile depth. Place your own textures directly into <code>/assets/textures/</code> and bump maps inside <code>/assets/bump_maps/</code></li>

```cpp	  
void load_materials(MaterialLibrary& mat_lib) {
	//bump map textures
	auto wood_bump = make_shared<image_texture>("assets/bump_maps/wood_bump_map.jpg");
	//... more bump maps as needed

	//add some predefined materials to the library
	mat_lib.add("wood_bumpy_texture", 
	make_shared<lambertian>(make_shared<image_texture>("assets/textures/fine-wood.jpg"), wood_bump, 2.0));
	//... add more materials as needed 
}
```
  <i>Note: Currently, Bump Mapping is not supported for <code>.obj</code> triangle meshes; this is planned for a future update.</i>
  <li><b>Zero-Copy Instancing:</b> Instead of duplicating heavy geometry, use a <code>material_instance</code> class to wrap a shared mesh with a specific material. This allows you to render hundreds of unique-looking objects while keeping only one copy of the mesh in RAM.</li>

```cpp	
void load_materials(MaterialLibrary& mat_lib) {
	auto wood_bump = make_shared<image_texture>("assets/bump_maps/wood_bump_map.jpg");
    
	//define a "bumpy" wood material in the library
	mat_lib.add("wood_bumpy", make_shared<lambertian>(
	make_shared<image_texture>("assets/textures/fine-wood.jpg"), 
	wood_bump, 2.0));
}

//in build_geometry: Reuse the same sphere geometry with different materials
auto sphere_geom = make_shared<sphere>(point3(0,0,0), 1.0, nullptr);
world.add(make_shared<material_instance>(sphere_geom, mat_lib.get("wood_bumpy")));
world.add(make_shared<material_instance>(sphere_geom, mat_lib.get("gold_mat")));
```
   </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b></b>Scene geometry</b></summary>
    <p>Place a geometry inside <code>build_geometry()</code> to compose your world. Use <code>material_instance</code> to apply shared materials to different shapes.</p>
    <ul>
      <li><b>Transformations:</b> Easily wrap objects in <code>translate, rotate_x/y/z</code>, and <code>scale</code> instances.</li>

```cpp		  
//cube
auto big_cube_geom = make_shared<cube>(point3(0.0, 0.0, 0.0), nullptr);
auto big_cube_instance = make_shared<material_instance>(big_cube_geom, mat_lib.get("foggy_glass"));
world.add(make_shared<translate>(big_cube_instance, point3(0.0, 1.0, 2.5)));
```
<li><b>Volumetric Fog:</b> Enable global environmental fog by setting <code>use_fog</code> to true and adjusting <code>fog_density</code> and <code>fog_color</code>.</li>

```cpp	
// - 5. environmental fog
if (use_fog) {
	//set radius and center of the fog volume (can be adjusted to fit the scene better)
	auto fog_boundary = make_shared<sphere>(point3(0.0, 0.0, 0.0), 50.0, nullptr);
	//fog density 0.1 is extremely high (impenetrable wall). 
	//values 0.001 - 0.02 gives best visual results.
	world.add(make_shared<constant_medium>(fog_boundary, fog_density, fog_color));
}
```
</ul>
  </details>
</ul>

### 🕹 Interactive UI Overview 
<ul>
  The engine features a custom-built, real-time diagnostic interface powered by Dear ImGui, providing deep insights into the path-tracing process and color pipeline. It supports Windows, Linux, and macOS.
</ul>

<ul style="list-style-type: none;">
  <details>
   <summary><b>Diagnostic G-Buffer Visualizer</b></summary>
    <p>Real-time inspection of internal engine data to verify scene integrity and material properties. These modes are directly linked to the <code>debug_mode</code> flags in the post-processing stage.</p>
    <ul>
      
| **Mode** | **Description** | **Technical Use** |
| :--- | :--- | :--- |
| **Albedo Pass** | *Raw material colors* | *Verify texture mapping and base reflectivity* |
| **Normal Pass** | *Surface orientation* | *Check for smoothing groups and geometry errors* |
| **Z-Depth Pass** | *Spatial distance* | *Debug focus distance for the Depth of Field (DoF) system* |
| **Luminance** | *Brightness map* | *Analyze the input for the Auto-Exposure algorithm* |
| **BVH Mode** | *Spatial Hierarchy* | *Audit tree health and node culling directly from the UI.* |

<p><code>IMAGE: obrazek z rgb/albedo/normals/z-depth/luminance/BVH Wireframe (Level 3 lub Leaves)</code></p>
  </ul>
  </details>
  </ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Real-Time Analytics & Control</b></summary>
    <p>Professional tools for lighting and exposure management.</p>
    
    Live Histogram & Channel Isolation
    GIF pokazujący, jak zmieniam ekspozycję suwakiem, a wykres histogramu „pływa” w czasie rzeczywistym.
  <i>Monitor the brightness distribution in real-time. This allows you to prevent highlight clipping and ensure that the <b>ACES Tone Mapping</b> has enough dynamic range to work with.</i>
    
  <ul>
    <li><b>Luminance Histogram:</b> Visualize the impact of scene lights on the final exposure.</li>
    <li><b>Channel Isolation:</b> Toggle <b>R, G, B,</b> views to perform precise noise analysis before the <b>Intel OIDN</b> pass.</li>
    <ul>
      <p><i>Technical Detail: Controlled via <code>debug_mode</code> flags in the post-processing shader.</i></p>
    </ul>
  </ul>
  </details>
  </ul>
   
<ul style="list-style-type: none;">
  <details>
    <summary><b>Fluid Interaction System</b></summary>
    <p>The engine features a deeply integrated communication layer between the Dear ImGui interface and the rendering core, focusing on a seamless user experience.</p>
    <p><code>IMAGE: G-Buffer GIF, jak przełączasz się między Normals a Final Render</code></p>
    <ul>
      <li><b>Smart Accumulator Sync:</b> To prevent constant frame flickering during UI interaction, the path-tracing accumulator only resets when a change is "finalized" (utilizing <code>IsItemDeactivatedAfterEdit</code>). This allows you to explore parameters smoothly, with the engine only committing to a full re-render once you release a slider.</li>
      <li><b>Non-Blocking Real-Time Updates:</b> Key parameters—including <b>Sun Position, Light Intensity,</b> and <b>Focus Distance</b>—provide immediate visual feedback, allowing for rapid look-dev and lighting adjustments without breaking the creative flow.</li>
      <li><b>Architectural Console & Logging:</b> A custom-built engine console provides categorized, filtered feedback directly within the viewport.</li>
      <ul>
        <li><b>Smart Categorization:</b><i> Logs are tagged as <code>[System]</code>, <code>[Config]</code>, or <code>[Debug]</code> for easy navigation and troubleshooting.</i></li>
        <li><b>Anti-Spam Logic:</b><i> Prevents log flooding during rapid parameter changes, ensuring that critical engine messages remain visible and the UI stays responsive even under heavy interaction.</i></li>
    </ul>
  </ul>
  </details>
  </ul>

### 🏗 Build & Requirements
<ul>
  This project is built using C++20 and utilizes CMake with vcpkg (as a submodule) for seamless, cross-platform dependency management. It supports Windows, Linux, and macOS.
</ul>
<ul style="list-style-type: none;">
  <details>
    <summary><b>Hardware & System Requirements</b></summary>
    <ul>
      <p><li><b>OS:</b> Windows 10/11, macOS (Intel/Apple Silicon), or Linux.</li>
      <li><b>CPU:</b> Multi-core processor with <b>OpenMP</b> support. <b>Note:</b> Intel OIDN requires a CPU with <b>SSE4.1</b> instructions or Apple Silicon (M1/M2/M3).</li>
      <li><b>GPU:</b> OpenGL 3.3 compatible(used for hardware-accelerated viewport display).</li>
      </p>
    </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Software Prerequisites</b></summary>
    <ul>
      <p><li><b>Compiler:</b> A C++20 compliant compiler (<b>MSVC 2022</b>, GCC 11+, or Clang 12+).</li>
      <li><b>Build System:</b> <a href="https://cmake.org/download/" target="_blank" rel="noopener noreferrer">CMake 3.15+</a></li>
      <li><b>Package Manager:</b> <a href="https://github.com/microsoft/vcpkg" target="_blank" rel="noopener noreferrer">vcpkg</a> (included as a git submodule).</li> 
      <li><b>Version Control:</b> <a href="https://git-scm.com/install/windows" target="_blank" rel="noopener noreferrer">Git</a> (required to manage vcpkg submodules and dependencies).</li> 
      </p>
    </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Dependencies</b></summary>
    <p>The project relies on the following libraries. Ensure they are installed or available in your environment:</p>
    <ul>
      <p>
        <li><b>Managed via vcpkg:</b> SDL3, Dear ImGui, Glad.</li>
        <li><b>External (Manual):</b> Intel Open Image Denoise (OIDN).</li>
        <li><b>System:</b> OpenMP (Multi-threading), OpenGL (Graphics API).</li>
      </p>
    </ul>
  </details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Build the Project</b></summary>
    <ul>
      <br>
      <div style="margin-left: 20px;">
        <b>1. Clone the Repository</b>
        <p>Clone the project along with the vcpkg submodule: </p>

```cpp	
git clone --recursive https://github.com/jarek1992/-Zenith.git
cd .\-Zenith\ 
```
  <p><i>Note: If you downloaded the repo without <code>--recursive</code>, run <code>git submodule update --init --recursive</code> inside the project folder to fetch vcpkg.</i></p>
  </p>
  </div>

  <div style="margin-left: 20px;">
  <b>2. Install Dependencies(System Specific)</b>
  <p>Since OIDN binaries are platform-specific and large, they are not included in the repository.</p>
  
  <ul style="list-style-type: disc; margin-left: 20px;">
  <b><li>Windows (Manual OIDN)</b></li>
    <ul>
      <div style="margin-left: 20px;">
        a. <b>Download</b> <code>oidn-2.3.0.x64.vc14.windows.zip</code> from <a href="https://github.com/RenderKit/oidn/releases" target="_blank" rel="noopener noreferrer">Intel OIDN Releases</a><br>
        b. <b>Extract</b> the contents so the structure looks like this:
        <ul style="margin-top: 0; padding-top: 0;">
          <li><code>libs/oidn/bin/</code> (contains <code>.dll</code> files)</li>
          <li><code>libs/oidn/include/</code> (contains headers)</li>
          <li><code>libs/oidn/lib/</code> (contains <code>.lib</code> files)</li>
        </ul>
      </div>
    </ul>
    <br>
    <li><b>macOS / Linux (System Package)</b><br>
      On these systems, OIDN is handled globally. Run:
      <ul style="margin-top: 0; padding-top: 0; margin-left: 20px;">
        <li><b>macOS:</b> <code>brew install openimagedenoise sdl3</code></li>
        <li><b>Linux (Ubuntu/Debian):</b> <code>sudo apt install libopenimagedenoise-dev libsdl3-dev</code></li>
      </ul>
    </li>
  </ul>
  <br>

<div style="margin-left: 20px;">
  <b>3. Bootstrap vcpkg:</b><br>
  Initialize the package manager (required for ImGui, Glad, and SDL3 on Windows):
  <ul style="margin-top: 0; padding-top: 0; margin-left: 20px;">
    <li><b>Windows:</b> 
      
    .\vcpkg\bootstrap-vcpkg.bat
  <li><b>macOS/Linux:</b>
    
    ./vcpkg/bootstrap-vcpkg.sh
  </ul>

  </p>
</div>
    <div style="margin-left: 20px;">
    <b>4. Configure & Install Dependencies:</b>
    <p>Run CMake to trigger <b>vcpkg</b> and configure the build system. This will automatically download and build <b>SDL3</b>, <b>ImGui</b>, and <b>Glad</b>.</p>

```cpp	
cmake -B build -S . "-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake"
```
  </p>
  </div>
    <div style="margin-left: 20px;">
    <b>5. Build the Project:</b>
    <p>Compile the executable in Release mode for maximum performance:</p>

```cpp	
cmake --build build --config Release
```
 </p>
   </div>
    <div style="margin-left: 20px;">
    <b>6. Run the Engine:</b><br>
    The build system automatically copies the necessary OIDN and SDL3 DLLs to the output folder. Launch the application:
      <ul style="margin-top: 0; padding-top: 0; margin-left: 20px;">
        <li><b>Windows:</b> 
          
    .\build\Release\zenith_path_tracer.exe
  <li><b>macOS/Linux:</b>
    
    ./build/zenith_path_tracer
  </ul>

<b><i>Note on Image Quality:</b> The engine features a built-in <b>ACES Tone Mapping</b> curve (see <code>common.hpp</code>) and <b>Auto-Exposure</b> logic. When running in <code>debug_mode::RED</code> or <code>GREEN</code>, you can observe the raw output of specific channels, while the main render utilizes Intel's AI Denoising for a noise-free experience.</i>
  
  </details>
</ul>
   
### 📈 Performance & Optimization
<ul>
  The engine is engineered for high-performance path tracing, balancing raw computational power with intelligent image reconstruction.
</ul>

<ul style="list-style-type: none;">
  <details>
   <summary><b>Multi-Threaded Rendering Core</b></summary>
    <p>The rendering engine is built to scale with your hardware, ensuring that every CPU cycle is utilized.</p>
    <ul>
      <li><b>OpenMP-Powered Scanline Parallelism:</b> The engine partitions the image into horizontal blocks of rows, distributed across all logical CPU cores. This ensures maximum hardware utilization and linear performance scaling.</li>
      <li><b>Adaptive Auxiliary Sampling:</b> To optimize bandwidth and compute cycles, the engine uses a decoupled sampling strategy. While the <b>Beauty Pass</b> uses full <code>samples_per_pixel</code>, the auxiliary buffers (Albedo, Normals, Z-Depth) are computed using a clamped subset of samples, drastically reducing overhead without sacrificing denoising quality.</li>
      <li><b>Live Progress Feedback:</b> Real-time synchronization between the rendering threads and the UI layer provides immediate visual feedback on the render's progress via atomic line-counting.</li>

      1. IMAGE: "Scanline Rendering Visualization" Pokazujący, jak obraz "zapełnia się" poziomymi pasami
      2. GIF: "Progressive Refinement" pasek postępu (Progress Bar) przesuwa się płynnie, a obraz staje się coraz wyraźniejszy(accumulator)
  </ul>
  </details>
  </ul>

  <ul style="list-style-type: none;">
  <details>
   <summary><b>AI-Accelerated Denosing (Intel OIDN)</b></summary>
    <p>Instead of relying on brute-force sampling (which can take hours), the engine uses industry-standard AI to "clean" the image.</p>
    <ul>
      <li><b>Sample Reduction:</b> Achieving noise-free results with <b>10x–50x fewer samples</b> per pixel compared to traditional path tracing.</li>
      <li><b>Pre-filter Pass:</b> The engine feeds Albedo and Normal buffers into the OIDN neural network, preserving sharp geometric edges while removing Monte Carlo variance.</li>
      <li><b>Performance Gain:</b> High-quality 1080p renders that would normally take minutes are usable in seconds.</li>

      IMAGE: Split-screen(moze z suwakiem jak da rade). Po lewej zaszumiony obraz (np. 10 próbek na piksel), po prawej ten sam obraz po przejściu przez OIDN.
      
  </ul>
  </details>
  </ul>

  <ul style="list-style-type: none;">
  <details>
   <summary><b>Smart Memory & Resource Management</b></summary>
    <ul>
      <p><li><b>vcpkg Manifest Mode:</b> All dependencies (SDL3, Glad, ImGui) are compiled with optimized release flags (<code>-O3 / /Ox</code>), ensuring zero overhead from third-party libraries.</li>
      <li><b>Accumulation Caching:</b> The engine detects when the camera or scene is static and accumulates light energy over time, avoiding redundant full-frame re-draws during idle periods.</li>
      <li><b>Fast Post-Processing:</b> The <b>ACES Tone Mapping</b> and <b>Auto-Exposure</b> passes are implemented as highly optimized per-pixel operations, adding negligible overhead to the final frame delivery.</li></p>
    </ul>
  </details>
  </ul>

  <ul style="list-style-type: none;">
  <details>
   <summary><b>Bounding Volume Hierarchy (BVH)</b></summary>
    <p>Traditional ray tracing checks every ray against every object (<i>O(N)</i> complexity) which is unsustainable for complex scenes with high primitive counts.</p>
    <ul>
      <li><b>Logarithmic Scaling:</b> By using an <i>O(logN)</i> traversal algorithm, the engine can handle scenes with thousands of primitives while maintaining high frame rates.</li>
      <li><b>Intersection Culling:</b> Rays that do not intersect a parent node's bounding box are immediately discarded, skipping all child nodes and primitives within.</li>
	  <li><b>Balanced Tree Construction:</b> Objects are sorted along the longest axis to ensure a balanced hierarchy, minimizing box overlap and maximizing traversal efficiency.</li>
     </ul><br>
	  <b>Integrated BVH Diagnostic Suite</b>
	  <p>The engine features a custom real-time visualizer to audit the health of the BVH tree directly from the <b>Engine Control Panel</b>.</p>
   <ul>
      
| **Feature** | **Description** |
| :--- | :--- |
| **BVH-Mode Toggle** | *Activates neon wireframe rendering for the active BVH layer* |
| **Level Sliders** | *Scans the hierarchy depth-by-depth to verify spatial partitioning.* | 
| **Leaf Isolation** | *Special mode (<code>Level: -1</code>) that highlights only the final leaf nodes<br> wrapping individual primitives.* |

    IMAGE: Obrazki z wlaczonymi roznymi levelami boxów.      
</ul>
</details>
</ul>

  <ul style="list-style-type: none;">
  <details>
   <summary><b>Dirty Flags & State Management</b></summary>
    <p>To avoid redundant calculations and ensure a fluid UI, the engine utilizes a <b>Dity Flag System</b> to track changes in the scene.</p>
    <ul>
      <li><b>Selective Re-renders:</b> Instead of blindly resetting the path-tracing accumulator every frame, the engine only triggers a reset when a "Dirty Flag" is raised (e.g., when a light is moved, a material is changed, or the camera is rotated).</li>
      <li><b>UI Synchronization:</b> Integrating with <b>Dear ImGui</b>, the engine uses <code>IsItemDeactivatedAfterEdit</code> to mark data as "dirty" only when the user finishes an interaction. This allows you to slide values smoothly.</li>
      <li><b>Resource Efficiency:</b> If no flags are dirty, the engine focuses 100% of its power on accumulating samples for the current frame, leading to rapid noise disappearance.</li>

      IMAGE: Nagranie ekranu np. zmiana koloru światła 
      (Pokazanie, że gdy tylko puścisz suwak, obraz zaczyna się 
      natychmiast "oczyszczać" (akumulować). 
      To udowodni działanie IsItemDeactivatedAfterEdit.)
      
  </ul>
  </details>
  </ul>

  <ul style="list-style-type: none;">
  <details>
   <summary><b>Performance Metrics (Example)</b></summary>
    <p><i>Tested on: Intel Core i7-12700F (12C/20T)/ 64GB RAM / Windows 11</i></p>
    <ul>
      
| **Feature** | **Status** | **Impact** |
| :--- | :--- | :--- |
| **Multi-threading** | *Enabled(OpenMP)* | *~95% CPU Utilization across 20 threads* |
| **SIMD Instructions** | *Enabled* | *Accelerated Ray-Sphere intersection math* |
| **Denoising** | *Intel OIDN 2.3* | *Clean images at 10-20 samples per pixel* |
| **UI Overhead** | *Minimal* | *Zero-copy frame buffer updates via Glad/OpenGL* |

    IMAGE: Mały zrzut ekranu z zakładki "Wydajność" w Windows, 
    gdzie wszystkie logiczne rdzenie (w moim przypadku 20 wątków i7-12700F) są obciążone na 100%.      
</ul>
</details>
</ul>

### 📸 Gallery & Showcase

<p>1.GIF showing real-time sun movement.</p>
<p>2. Screenshots of the G-Buffer passes.</p>
<br>
<p>Final renders comparing ACES On/Off and Bloom effects.</p>
<ul>
  <p>3. Indoor Scene: Highlighting Global Illumination, soft shadows from area lights, and color bleeding.</p>
  <p>4. Macro Photography: Showcasing the Thin-Lens model with extreme Bokeh and high-precision focus.</p>
  <p>5. Exterior Sunlight: Demonstrating the Astronomical Day-Night cycle and Rayleigh Scattering.</p>
</ul>

### 🖥 Interface in Action

    GIF-a pokazujący jak slider godziny (słońce się porusza) lub jak zmienia sie tryby G-Buffera.
### 📊 Real-time Analytics
- <b>Luminance Histogram:</b> A real-time distribution of pixel brightness, used to calibrate the Auto-Exposure and prevent highlight clipping.
<ul>
  <p><code>IMAGE/GIF: screenshot z histogramem i aktywnymi kanałami RGB.</code></p>
</ul>

- <b>G-Buffer Suite:</b> Toggle between Albedo, Normals, and Z-Depth passes to inspect the scene's geometric health.
<ul>
  <p><code>IMAGE/GIF: przelaczanie sie miedzy passami albedo, normals i z-depth.</code></p>
</ul>

- <b>Frame Profiler:</b> Engineered for maximum throughput. The engine utilizes all logical CPU cores via OpenMP, providing real-time progress tracking via atomic line counters for smooth UI updates.
<ul>

  ```cpp	
  // Every 10 lines update progress bar for atomic safety
  if (local_lines_done % 10 == 0 || j == end_y - 1) {
  	// fetch_add ensures thread-safe updates across all CPU cores
  	this->lines_rendered.fetch_add(local_lines_done);
  	local_lines_done = 0; // reset local thread counter
  }
```
</ul>

### 🗺 Future Roadmap
Where the project is headed. Contributions and suggestions are welcome!

- [ ] Advanced Geometry Support: Extending Bump Mapping to <code>.obj</code> triangle meshes (currently limited to primitives like Spheres and Boxes).
- [ ] ImGui UI: Improve the default Dear ImGui outlook for users.
- [ ] GPU Acceleration: Porting the core kernels to CUDA or OptiX for 100x performance gains.
- [ ] Advanced Denoising: Adding support for NVIDIA DLSS 3.5 (Ray Reconstruction).
- [ ] Material Extensions: Implementation of Subsurface Scattering (SSS) for skin and wax, and Clear-Coat for car paints.
- [ ] Complexity: Full Wavefront Path Tracing architecture to better handle divergent rays.
- [ ] Animation: Integrated timeline for keyframing camera paths and light transitions.

### Updates:
<i>to be continued ... </i>
