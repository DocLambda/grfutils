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

option(BUILD_DOC "Generate API documentation (requires doxygen)." ON)

if(BUILD_DOC)
	find_package(Doxygen REQUIRED)
	configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
			${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
		      )
	add_custom_target(doc
			  ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
			  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			  COMMENT "Generating API documentation with Doxygen"
			  VERBATIM
	)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html
		DESTINATION share/doc
		OPTIONAL
	)
endif(BUILD_DOC)

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
