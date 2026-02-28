cmake_minimum_required(VERSION 3.15)
project(ray_tracer)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# Set C++20 standard AND OpenMP
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenMP REQUIRED)

# VCPKG libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# OIDN is handled separately because it may not be available in vcpkg or the system, so we will check for it and use a local copy if necessary.
set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn")

# Attempt to find OIDN in the system first, if not found, use the local copy
find_package(OpenImageDenoise QUIET)
if (NOT OpenImageDenoise_FOUND)
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    if (MSVC)
        set(OIDN_LINK_TARGET "${OIDN_DIR}/lib/OpenImageDenoise.lib")
    else()
        set(OIDN_LINK_TARGET "-L${OIDN_DIR}/lib -lOpenImageDenoise")
    endif()
else()
    set(OIDN_INCLUDE "${OpenImageDenoise_INCLUDE_DIRS}")
    set(OIDN_LINK_TARGET OpenImageDenoise)
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
    OpenMP::OpenMP_CXX
    ${OIDN_LINK_TARGET}
    $<$<PLATFORM_ID:Windows>:opengl32>
    $<$<PLATFORM_ID:Darwin>:"-framework OpenGL" "-framework Cocoa">
)

# Copy DLL files for Windows(only)
if (MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OIDN_DIR}/bin/OpenImageDenoise.dll"
        "${OIDN_DIR}/bin/tbb12.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Diagnostic messages
message(STATUS "Configured for SDL3 + OpenGL + OIDN via vcpkg")