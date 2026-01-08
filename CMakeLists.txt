cmake_minimum_required(VERSION 3.11.0)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# vcpkg integration
set(CMAKE_TOOLCHAIN_FILE 
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
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
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui") # Change for ImGui destination
include_directories(${IMGUI_DIR})
include_directories(${IMGUI_DIR}/backends)
include_directories(${IMGUI_DIR}/misc/stb)

# Path to stb_image_write
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/stb")

# Path to tinyobjloader
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/TinyObjLoader")

# Path to OIDN (denoiser)
set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn")
include_directories("${OIDN_DIR}/include")
link_directories("${OIDN_DIR}/lib")
message(STATUS "Searching for OIDN in: ${OIDN_DIR}")

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
OpenMP::OpenMP_CXX
"${OIDN_DIR}/lib/OpenImageDenoise.lib"
)

# Copy automatic all the .dll files to the bin folder with file .exe 
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${OIDN_DIR}/bin/OpenImageDenoise.dll"
    "${OIDN_DIR}/bin/OpenImageDenoise_core.dll"
    "${OIDN_DIR}/bin/OpenImageDenoise_device_cpu.dll"  
    "${OIDN_DIR}/bin/OpenImageDenoise_device_cuda.dll"
    "${OIDN_DIR}/bin/tbb12.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)