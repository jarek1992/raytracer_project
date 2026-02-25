## 🚀 Interactive C++ Path Tracing Engine
A high-performance, physically-based path tracing engine built with C++20. This project features a robust real-time diagnostic UI, sophisticated environmental lighting, and a professional post-processing pipeline. 

<br>

<code>[Image: Główny render Twojego silnika – np. scena z efektownym oświetleniem, odbiciami i bloomem]</code>

<br>

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
        <li><b>Integration Method:</b> Monte Carlo (stochastic sampling of light paths).</li>
        <li><b>Hardware Acceleration(CPU):</b> <b>OpenMP</b> (parallelized computation across all available processor threads).</li>
        <li><b>Data Structures:</b> <b>BVH(Bounding Volume Hierarchy)</b> – optimizes ray-object intersection tests from <i>O(N)</i> to <i>O(logN)</i>.</li>
        <ul>
          <br>
          <code> IMAGE: Left: A complex mesh partitioned into AABBs (Axis-Aligned Bounding Boxes). Right: The resulting tree structure used for O(logN) traversal.</code>
        </ul>
      <br>
      <li>
        <b>Memory Management:</b> Dirty Flag System (<code>needs_update</code>, <code>needs_ui_sync</code>) – intelligent buffer reloading triggered only upon parameter changes.

      // Example of smart state synchronization
      if (ImGui::SliderFloat("Aperture", &cam.aperture, 0.0f, 0.5f)) {
          my_post.needs_update = true; // Trigger post-process recalculation
      }
      
      if (ImGui::IsItemDeactivatedAfterEdit()) {
          reset_accumulator(); // Only reset samples when user finishes interaction
      }
      
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
| Sample Jittering | *High-quality sub-pixel anti-aliasing* | *Stratified Sampling (per pixel)* |

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
        <li><b>Technology:</b><i> Native support for 32-bit .hdr files (IBL) providing a massive range of luminance data.</i></li>
        <li><b>3-Axis Transformation:</b><i> Full spherical orientation control using Yaw, Pitch, and Roll to align the environment with your scene geometry perfectly.</i></li>
        <li><b>Asset Management:</b><i> Integrated file observer allows for dynamic refreshing of the HDRI library. Add new maps to the directory and select them in-app without a restart.</i></li>
        <br>
      </ul>
      <li><b>Astronomical Daylight System:</b></li>
      <p>This module simulates the sun's position and color based on real-world celestial mechanics. It offers two distinct modes of operation:</p>
      <ul>
        <li><b>Astronomical Mode:</b><i> Calculates the sun's position automatically using geographical and temporal data.</i></li>
        <ul style="list-style-type: disc; margin-left: 20px;">
          <li><b>Parameters:</b><i> Latitude (coordinates), Day of the Year (1-365), and Time of Day (0-24h).</i></li>
          <li><b>Celestial Math:</b><i> Implementation of solar declination, hour angle, azimuth, and elevation algorithms to ensure pinpoint accuracy.</i></li>
          <br>
        </ul>
        <li><b>Manual Mode:</b><i> Provides direct control over the sun's direction vector for artistic lighting setups.</i></li>
        <li><b>Auto-Sun Color:</b><i> Dynamically adjusts the solar spectrum based on the altitude angle. It transitions from warm, golden hues during sunrise/sunset to cool, crisp white at the zenith, driven by the vertical Y component of the sun vector.</i></li>
        <br>
      </ul>
      <li><b>Solid Background:</b></li>
      <p>Designed for product photography and clean architectural presentations.</p>
      <ul style="list-style-type: disc; margin-left: 20px;">
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
        
| **Material** | **Physical Property** | **Key Features** |
| :--- | :--- | :--- |
| Lambertian | *Ideal Diffuse* | *Simulates matte surfaces with perfect light scattering.<br><b>Supported(Albedo Maps)</b>* |
| Metal | *Specular Reflection* | *Includes a <b>Fuzz</b> parameter to control surface roughness/blurriness of reflections.<br><b>Supported (Fuzz/Color Maps)</b>* |
| Dielectric | *Refraction* | *Handles transparent materials like glass or water with <b>IOR</b> (Index of Refraction) and Total Internal Reflection.<br><b>Procedural Tinting</b>* |
| Emissive | *Light Emission* | *Turns any geometry into a physical light source (<b>Area Light</b>) with adjustable radiance.<br><b>Supported (Light Maps)</b>* |

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
  <li><b>Bump Mapping(Beta):</b><i> Preliminary support for Bump Mapping is available for basic primitives, allowing for fine-grained surface detail without increasing polygon count.</i></li>
    <ul>
      <li><i>Note: Currently, Bump Mapping is not supported for .obj triangle meshes; this is planned for a future update.</i></li>
    </ul>
  </ul>
</details>
</ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Cinematic Post-Process Pipeline</b></summary>
    <p>Beyond path tracing, the engine includes a high-performance post-processing stack to achieve a production-ready look.</p>
    <ul>
      <li><b>ACES Tone Mapping:</b><i> Implementation of the Academy Color Encoding System to transform High Dynamic Range (HDR) data into cinematic Low Dynamic Range (LDR) output.</i></li>
      <li><b>Bloom Engine:</b><i> A physically-inspired glow effect that extracts highlights and bleeds them into surrounding pixels using a configurable threshold and blur radius.</i></li>
      <li><b>Exposure Control:</b>
        <ul>
          <li><b>Auto-Exposure:</b> Note: Currently, Bump Mapping is not supported for .obj triangle meshes; this is planned for a future update.</li>
          <li><b>EV Compensation:</b>Photographic control allowing for ±5.0 stops of brightness adjustment.</li>
        </ul>
      </li>
    </ul>
  </details>
</ul>

### 🕹 Interactive UI Overview

<ul style="list-style-type: none;">
  <details>
   <summary><b>Diagnostic G-Buffer Visualizer</b></summary>
    <p>Real-time inspection of internal engine passes to verify scene integrity.</p>
    <ul>
        
| **Albedo Pass** | **Normal Pass** | **Z-Depth Pass** |
| :--- | :--- | :--- |
| *Raw material colors* | *Surface orientation* | *Spatial distance* |

  </ul>
  </details>
  </ul>

<ul style="list-style-type: none;">
  <details>
    <summary><b>Real-Time Analytics & Control</b></summary>
    <p>Professional tools for lighting and exposure management.</p>
    
    [!TIP]
    Live Histogram & Channel Isolation
    Tutaj idealnie pasuje GIF pokazujący, jak zmieniasz ekspozycję suwakiem, a wykres histogramu „pływa” w czasie rzeczywistym.
    
  <ul>
    <li><b>Luminance Histogram:</b><i> Monitor brightness distribution to prevent highlight clipping.</i></li>
    <li><b>Debug Channels:</b><i> Toggle <b>R, G, B,</b> and <b>Luminance</b> views for precise noise analysis. </i></li>
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
    <ul>
      <li><b>Smart Accumulator Sync:</b><i> To prevent constant frame flickering during UI interaction, the path-tracing accumulator only resets when a change is "finalized" (utilizing <code>IsItemDeactivatedAfterEdit</code>). This allows you to explore parameters smoothly, with the engine only committing to a full re-render once you release a slider.</i></li>
      <li><b>Non-Blocking Real-Time Updates:</b><i> Key parameters—including <b>Sun Position, Light Intensity,</b> and <b>Focus Distance</b>—provide immediate visual feedback, allowing for rapid look-dev and lighting adjustments.</i></li>
      <li><b>Architectural Console & Logging:</b><i> A custom-built engine console provides categorized, filtered feedback.</i></li>
      <ul>
        <li><b>Smart Categorization:</b><i> Logs are tagged as <code>[System]</code>, <code>[Config]</code>, or <code>[Debug]</code> for easy navigation.</i></li>
        <li><b>Anti-Spam Logic:</b><i> Prevents log flooding during rapid parameter changes, ensuring that critical engine messages remain visible and the UI stays responsive.</i></li>
    </ul>
  </ul>
  </details>
  </ul>
 

### 🏗 Build & Requirements

### 📈 Performance & Optimization

### 📸 Gallery & Showcase

### 🗺 Future Roadmap (Gdzie projekt zmierza).




🖥 Interface in Action

    Tip: Tutaj wstaw GIF-a pokazującego jak przesuwasz slider godziny (słońce się porusza) lub jak zmieniasz tryby G-Buffera.
📊 Real-time Analytics

    Tip: Tutaj wstaw screenshot z histogramem i aktywnymi kanałami RGB.
































#RayTracer
<img width="1196" height="674" alt="Image" src="https://github.com/user-attachments/assets/f461abfb-13b8-4e8f-96cf-e87e6a77c402" />
