## 🚀 Interactive C++ Path Tracing Engine
A high-performance, physically-based path tracing engine built with C++20. This project features a robust real-time diagnostic UI, sophisticated environmental lighting, and a professional post-processing pipeline. 

### 🛠 Core Technical Features
<ul>
  <details>
    <summary><b>Engine Specifications</b></summary>
    <ul>
      <p><li><b>Language:</b> C++20 (utilizing modern standards: std::clamp, std::shared_ptr, and advanced lambdas)</li>
        <li><b>Rendering Model:</b> Progressive Path Tracing (real-time sample accumulation)</li>
        <li><b>Integration Method:</b> Monte Carlo (stochastic sampling of light paths)</li>
        <li><b>Hardware Acceleration(CPU):</b> OpenMP (parallelized computation across all available processor threads)</li>
        <li><b>Data Structures:</b> Progressive Path Tracing (real-time sample accumulation)</li>
        <li><b>Memory Management:</b> Dirty Flag System (needs_update, needs_ui_sync) – intelligent buffer reloading triggered only upon parameter changes</li>
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
        <li><b>Technology:</b> Native support for 32-bit .hdr files (IBL) providing a massive range of luminance data.</li>
        <li><b>3-Axis Transformation:</b> Full spherical orientation control using Yaw, Pitch, and Roll to align the environment with your scene geometry perfectly.</li>
        <li><b>Asset Management:</b> Integrated file observer allows for dynamic refreshing of the HDRI library. Add new maps to the directory and select them in-app without a restart.</li>
        <br>
      </ul>
      <li><b>Astronomical Daylight System:</b></li>
      <p>This module simulates the sun's position and color based on real-world celestial mechanics. It offers two distinct modes of operation:</p>
        <li><b>Astronomical Mode:</b> Calculates the sun's position automatically using geographical and temporal data.</li>
        <ul>
          <li><b>Parameters:</b> Latitude (coordinates), Day of the Year (1-365), and Time of Day (0-24h).</li>
          <li><b>Celestial Math:</b> Implementation of solar declination, hour angle, azimuth, and elevation algorithms to ensure pinpoint accuracy.</li>
        </ul>
        <li><b>Manual Mode:</b> Provides direct control over the sun's direction vector for artistic lighting setups.</li>
        <li><b>Auto-Sun Color:</b> Dynamically adjusts the solar spectrum based on the altitude angle. It transitions from warm, golden hues during sunrise/sunset to cool, crisp white at the zenith, driven by the vertical Y component of the sun vector.</li>
        <br>
      </ul>
      <li><b>Solid Background:</b></li>
      <p>Designed for product photography and clean architectural presentations.</p>
        <li><b>Control:</b> Full RGB spectrum selection via a precision color picker.</li>
        <li><b>Intensity:</b> Adjustable background radiance, allowing for neutral studio setups that don't overpower the scene's primary light sources.</li>
      </ul>
  </details>
</ul>



### 🕹 Interactive UI Overview

### 🏗 Build & Requirements

### 📈 Performance & Optimization

### 📸 Gallery & Showcase
🖥 Interface in Action

    Tip: Tutaj wstaw GIF-a pokazującego jak przesuwasz slider godziny (słońce się porusza) lub jak zmieniasz tryby G-Buffera.
📊 Real-time Analytics

    Tip: Tutaj wstaw screenshot z histogramem i aktywnymi kanałami RGB.
































#RayTracer
<img width="1196" height="674" alt="Image" src="https://github.com/user-attachments/assets/f461abfb-13b8-4e8f-96cf-e87e6a77c402" />
