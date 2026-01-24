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

# Find OpenMP
find_package(OpenMP REQUIRED)

#Paths to libraries(Imgui,SDL3,OpenGL,OIDN)
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(IMGUI_DIR "${LIBS_DIR}/imgui")
set(SDL3_DIR "${LIBS_DIR}/SDL3-3.4.0")
set(GLAD_DIR "${LIBS_DIR}/glad")
set(OIDN_DIR "${LIBS_DIR}/oidn")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL3_DIR}/include
    ${GLAD_DIR}/include
    ${LIBS_DIR}/stb
    ${LIBS_DIR}/TinyObjLoader
    ${OIDN_DIR}/include
)

# Add sources
add_executable(ray_tracer 
    main.cpp
    stb_impl.cpp
    ${GLAD_DIR}/src/glad.c #OpenGL loader
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp #SDL3 backend
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp #OpenGL3 backend
)

# Diagnostic messages
message(STATUS "Searching for OIDN in: ${OIDN_DIR}")

# Link libraries to the ray_tracer executable
target_link_libraries(ray_tracer 
    OpenMP::OpenMP_CXX
    "${OIDN_DIR}/lib/OpenImageDenoise.lib"
    "${SDL3_DIR}/lib/x64/SDL3.lib" 
    opengl32 # OpenGL on Windows
)

# Copy automatic all the .dll files to the bin folder with file .exe 
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${OIDN_DIR}/bin/OpenImageDenoise.dll"
    "${OIDN_DIR}/bin/OpenImageDenoise_core.dll"
    "${OIDN_DIR}/bin/OpenImageDenoise_device_cpu.dll"  
    "${OIDN_DIR}/bin/OpenImageDenoise_device_cuda.dll"
    "${OIDN_DIR}/bin/tbb12.dll"
    "${SDL3_DIR}/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

message(STATUS "Configured for SDL3 + OpenGL + OIDN")