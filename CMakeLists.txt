cmake_minimum_required (VERSION 3.5)
project (demo)
#add_subdirectory(src)

option(ARM "arm-linux-gcc" OFF)

if (ARM)
	SET(CMAKE_C_COMPILER arm-linux-gcc)
	SET(PROJECT_NAME server_arm)
else()
	SET(PROJECT_NAME server_linux)
endif()

include_directories(include)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
aux_source_directory(./src SRC)
add_compile_options(-g -Wall)
add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} jpeg)
target_link_libraries(${PROJECT_NAME} pthread)