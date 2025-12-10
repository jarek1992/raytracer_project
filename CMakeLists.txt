cmake_minimum_required(VERSION 3.11.0)
project(ray_tracer)

# Flags for the compiler
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan setup
set(VULKAN_SDK "$ENV{VULKAN_SDK}")
include_directories(${VULKAN_SDK}/include)
link_directories(${VULKAN_SDK}/lib)

# vcpkg integration
set(CMAKE_TOOLCHAIN_FILE 
    "C:/Users/Jarek/Desktop/raytracer_project/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "vcpkg toolchain file"
)

# Path to ImGui files
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui") # Change for ImGui destination
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)
include_directories(${IMGUI_DIR}/misc/stb)

# Path to stb_image_write
include_directories("${CMAKE_SOURCE_DIR}/libs/stb")

# Find and link Vulkan and GLFW libraries
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# HDR map
# file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})


# Add sources
add_executable(ray_tracer 
    main.cpp
    stb_impl.cpp
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_vulkan.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
)

# Link Vulkan and GLFW to the ray_tracer executable
target_link_libraries(ray_tracer Vulkan::Vulkan glfw)