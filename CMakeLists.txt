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
    add_compile_options(/utf-8)
else()
	# using linux/macOS OpenMP flags often use find_package(OpenMP) to set the appropriate flags for the platform
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)

#Paths to libraries(Imgui,SDL3,OpenGL,OIDN)
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")
set(IMGUI_DIR "${LIBS_DIR}/imgui")
set(SDL3_DIR "${LIBS_DIR}/SDL3-3.4.0")
set(GLAD_DIR "${LIBS_DIR}/glad")
set(OIDN_DIR "${LIBS_DIR}/oidn")

# Detecting OIDN 
find_package(OpenImageDenoise QUIET)

if (OpenImageDenoise_FOUND)
	message(STATUS "Found OIDN in system: ${OpenImageDenoise_DIR}")
    set(OIDN_LINK_TARGET OpenImageDenoise)
    set(OIDN_INCLUDE "${OpenImageDenoise_INCLUDE_DIRS}")
else()
	message(STATUS "OIDN not found in system, using local path: ${OIDN_DIR}")
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    if (MSVC)
        set(OIDN_LINK_TARGET "${OIDN_DIR}/lib/OpenImageDenoise.lib")
    else()
        set(OIDN_LINK_TARGET "-L${OIDN_DIR}/lib -lOpenImageDenoise")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL3_DIR}/include
    ${GLAD_DIR}/include
    ${LIBS_DIR}/stb
    ${LIBS_DIR}/TinyObjLoader
    ${OIDN_INCLUDE}
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

if (APPLE)
	# MacOS links frameworks and libraries .dylib
    target_link_libraries(ray_tracer 
		OpenMP::OpenMP_CXX
		${OIDN_LINK_TARGET}
        "-L${SDL3_DIR}/lib -lSDL3"
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
	)
elseif (MSVC)
    # Windows links .lib files and .dll for OIDN and SDL3
    target_link_libraries(ray_tracer 
        OpenMP::OpenMP_CXX
        ${OIDN_LINK_TARGET}
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
endif()

# Diagnostic messages
message(STATUS "Configured for SDL3 + OpenGL + OIDN")