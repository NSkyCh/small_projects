cmake_minimum_required (VERSION 3.5)
project (demo)
#add_subdirectory(src)

option(ARM_LINUX "print one message" OFF)

if (ARM_LINUX)
	SET(CMAKE_C_COMPILER "arm-linux-gcc")
endif()

include_directories(include)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
aux_source_directory(./src SRC)
add_compile_options(-g -Wall)
add_executable(server ${SRC})

target_link_libraries(server jpeg)
target_link_libraries(server pthread)