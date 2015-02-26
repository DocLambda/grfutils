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

include_directories (${PROJECT_SOURCE_DIR}/src)

link_directories(${PROJECT_BINARY_DIR}/src)

add_executable(grfctl grfctl.c grf_scan.c grf_request.c)

target_link_libraries(grfctl grf m)

install(TARGETS grfctl DESTINATION bin)
