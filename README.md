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

<ul>
  <details>
    <summary><b>Environment & Lighting Systems</b></summary>
    <summary>The engine provides a versatile lighting suite, allowing users to switch between physical sky simulations, image-based lighting, and studio backgrounds.</summary>
    <br>
    <ul> 
      <p><li><b>HDRI Maps(IBL):</b> C++20 (utilizing modern standards: std::clamp, std::shared_ptr, and advanced lambdas)</li>
        <li><b>Astronomical Daylight System:</b> Progressive Path Tracing (real-time sample accumulation)</li>
        <li><b>Solid Background:</b> Monte Carlo (stochastic sampling of light paths)</li>
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
