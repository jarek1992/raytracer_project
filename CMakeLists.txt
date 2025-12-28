cmake_minimum_required(VERSION 3.11.0)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# vcpkg integration
set(CMAKE_TOOLCHAIN_FILE 
    "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "vcpkg toolchain file"
)

project(ray_tracer)

# Flags for the compiler
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/openmp)
endif()

# Find and link Vulkan and GLFW libraries, OpenMP
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenMP REQUIRED)

# Vulkan setup
set(VULKAN_SDK "$ENV{VULKAN_SDK}")
include_directories(${VULKAN_SDK}/include)
link_directories(${VULKAN_SDK}/lib)

# Path to ImGui files
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui") # Change for ImGui destination
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)
include_directories(${IMGUI_DIR}/misc/stb)

# Path to stb_image_write
include_directories("${CMAKE_SOURCE_DIR}/libs/stb")

# Path to tinyobjloader
include_directories("${CMAKE_SOURCE_DIR}/libs/TinyObjLoader")

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
target_link_libraries(ray_tracer 
Vulkan::Vulkan 
glfw
)