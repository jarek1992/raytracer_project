cmake_minimum_required(VERSION 3.15)
project(ray_tracer)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# Set C++20 standard AND OpenMP
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenMP REQUIRED)

# VCPKG libraries(SDL3, ImGui, Glad)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# Intel OIDN library manual setup
if (WIN32)
    # Twój sprawdzony setup dla Windows
    set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn")
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    set(OIDN_LIB "${OIDN_DIR}/lib/OpenImageDenoise.lib")
    message(STATUS "Using local OIDN binaries for Windows")
else()
    # Standardowe szukanie na Linux/macOS (np. po 'brew install openimagedenoise')
    find_package(OpenImageDenoise REQUIRED)
    set(OIDN_INCLUDE "") # find_package sam doda œcie¿ki do targetu
    set(OIDN_LIB OpenImageDenoise)
    message(STATUS "Using system OIDN via find_package")
endif()

# add sources 
add_executable(${PROJECT_NAME} 
    main.cpp
    stb_impl.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OIDN_INCLUDE}
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/TinyObjLoader"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
   SDL3::SDL3 
   imgui::imgui 
   glad::glad
   ${OIDN_LIB}
   OpenMP::OpenMP_CXX
   $<$<PLATFORM_ID:Windows>:opengl32>
   $<$<PLATFORM_ID:Darwin>:"-framework OpenGL" "-framework Cocoa">
)

# Automatic DLLs copying for OIDN on Windows
if (MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OIDN_DIR}/bin/OpenImageDenoise.dll"
        "${OIDN_DIR}/bin/OpenImageDenoise_core.dll"
        "${OIDN_DIR}/bin/OpenImageDenoise_device_cpu.dll"
        "${OIDN_DIR}/bin/tbb12.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Diagnostic messages
message(STATUS "Full vcpkg build: SDL3 + ImGui + Glad")