#
#    CMakeLists.txt - cmake for StormLib MPQ archiving utility
#    Copyright (C) 2010  Pali Rohár <pali.rohar@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

project(smpq)
cmake_minimum_required(VERSION 2.6)

option(WITH_CMD "Enable smpq console application" ON)
option(WITH_KDE "Enable KDE4 kio_smpq plugin" ON)

# TODO: Find StormLib in system

set(smpq_SRCS
	append.cpp
	extract.cpp
	filetime.c
	main.c
	mkpath.c
	print.c
	remove.cpp
)

set(kio_smpq_SRCS
	kio_smpq.cpp
)

if(WITH_CMD)

	add_executable(smpq ${smpq_SRCS})
	target_link_libraries(smpq StormLib)
	install(TARGETS smpq DESTINATION bin)

endif(WITH_CMD)

if(WITH_KDE)

	find_package(KDE4 REQUIRED)
	include(KDE4Defaults)

	include_directories(${KDE4_INCLUDES})

	kde4_add_plugin(kio_smpq ${kio_smpq_SRCS})
	target_link_libraries(kio_smpq ${KDE4_KIO_LIBS} StormLib)

	install(TARGETS kio_smpq DESTINATION ${PLUGIN_INSTALL_DIR})
	install(FILES smpq.protocol DESTINATION ${SERVICES_INSTALL_DIR})

endif(WITH_KDE)
