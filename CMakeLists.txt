cmake_minimum_required(VERSION 3.15)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
project(zenith_path_tracer)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# Set C++20 standard AND OpenMP
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix for OpenMP on macOS using AppleClang
if (APPLE)
    # check if Homebrew is installed in standard location (M1/M2/M3)
    if (EXISTS "/opt/homebrew/opt/libomp")
        set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
        set(OpenMP_C_LIB_NAMES "omp")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
        
        # Paths to headers and libs for compilator 
        include_directories("/opt/homebrew/opt/libomp/include")
        link_directories("/opt/homebrew/opt/libomp/lib")
    endif()
endif()

find_package(OpenMP REQUIRED)

# VCPKG libraries(SDL3, ImGui, Glad)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# Intel OIDN setup (multi-platform)
if (WIN32)
    set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn")
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    set(OIDN_LIB "${OIDN_DIR}/lib/OpenImageDenoise.lib")
    message(STATUS "Using local OIDN binaries for Windows")
elseif(APPLE)
    #macOS requires .dylib files (download x86_64 version or arm64 OIDN)
    set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn_mac")
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    set(OIDN_LIB "${OIDN_DIR}/lib/libOpenImageDenoise.dylib")
    message(STATUS "Using local OIDN binaries for macOS")
else()
    set(OIDN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/oidn_linux")
    set(OIDN_INCLUDE "${OIDN_DIR}/include")
    #link directly to .so file
    set(OIDN_LIB "${OIDN_DIR}/lib/libOpenImageDenoise.so")
    message(STATUS "Using local OIDN binaries for Linux")
endif()

# Add sources 
add_executable(${PROJECT_NAME} 
    main.cpp
    stb_impl.cpp
)

# Unify paths for vcpkg for different operating system
if(APPLE)
set(VCPKG_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/arm64-osx/include")
    # if macOS with Intel, vcpkg uses x64-osx:
    if(NOT EXISTS "${VCPKG_INCLUDE_PATH}")
        set(VCPKG_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-osx/include")
    endif()
else()
    set(VCPKG_INCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-linux/include")
endif()


# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OIDN_INCLUDE}
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/TinyObjLoader"
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/imgui"
)

# Link libraries
if(APPLE)
    set(MAC_FRAMEWORKS "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
   SDL3::SDL3 
   imgui::imgui 
   glad::glad
   ${OIDN_LIB}
   OpenMP::OpenMP_CXX 
   $<$<PLATFORM_ID:Windows>:opengl32>
   $<$<PLATFORM_ID:Darwin>:${MAC_FRAMEWORKS}>
)

# RPATH for macOS/Linux
if(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../lib;@loader_path/.")
endif()

# Automatic DLLs copying for OIDN on Windows
if (MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OIDN_DIR}/bin/OpenImageDenoise.dll"
        "${OIDN_DIR}/bin/OpenImageDenoise_core.dll"
        "${OIDN_DIR}/bin/OpenImageDenoise_device_cpu.dll"
        "${OIDN_DIR}/bin/tbb12.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying OIDN Runtime DLLs..."
    )
endif()

# Diagnostic messages
message(STATUS "Build configured for: SDL3, ImGui, Glad, OIDN")