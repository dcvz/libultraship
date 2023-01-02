cmake_minimum_required(VERSION 3.16.0)

project(libultraship LANGUAGES C CXX)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    enable_language(OBJCXX)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/Utils.cmake)

# use vcpkg for dependencies on windows if flag is set
if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND USE_AUTO_VCPKG)
    include(cmake/automate-vcpkg.cmake)

    set(VCPKG_TRIPLET x64-windows-static)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)

    vcpkg_bootstrap()
    vcpkg_install_packages(zlib bzip2 SDL2 GLEW)
endif()

## Setup GL for subprojects
if (USE_OPENGLES)
    find_library(OPENGL-LIB NAMES GLESv2)
    if (NOT OPENGL-LIB)
        message(FATAL_ERROR "USE_OPENGLES=1, but libGLESv2 could not be found")
    endif()
else()
    find_package(OpenGL QUIET)
    set(OPENGL-LIB ${OPENGL_opengl_LIBRARY})
endif()

# setup subprojects
add_subdirectory("extern")
add_subdirectory("src")

