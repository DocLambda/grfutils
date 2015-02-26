#
# This file is part of the grfutils project.
#
# Copyright (c) 2014-2015 Sven Rebhan <odinshorse@googlemail.com>
#
# grfutils is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# grfutils is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with grfutils.  If not, see <http://www.gnu.org/licenses/>.

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
