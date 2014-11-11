cmake_minimum_required(VERSION 2.8.3)

project(GRFUTILS)

set(GRFUTILS_VERSION_MAJOR    0)
set(GRFUTILS_VERSION_MINOR    1)
set(GRFUTILS_VERSION_REVISION 0)

#configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")

# Debug/Release flags
set(CMAKE_C_FLAGS_DEBUG "-DGRFUTILS_DEBUG")

# Compiler flags
add_definitions("-Wall")
add_definitions("-std=gnu99")
add_definitions("-D_GNU_SOURCE")

# Subdirectories
add_subdirectory(src)
add_subdirectory(bin)
