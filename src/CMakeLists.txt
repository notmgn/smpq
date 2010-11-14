set(smpq_SRCS
	append.cpp
	extract.cpp
	filetime.c
	main.c
	mkpath.c
	print.c
	remove.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(smpq ${smpq_SRCS})
target_link_libraries(smpq StormLib)
