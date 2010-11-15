project(smpq)
cmake_minimum_required(VERSION 2.8)

set(smpq_SRCS
	append.cpp
	extract.cpp
	filetime.c
	main.c
	mkpath.c
	print.c
	remove.cpp
)

add_executable(smpq ${smpq_SRCS})
target_link_libraries(smpq StormLib)
