cmake_minimum_required(VERSION 3.22)

# set the project name
project(data_format VERSION 1.0)

# add the executable
add_library(${PROJECT_NAME} SHARED
			./source/mesh_format/utils.c
			./include/data_format/internal/module.h)
			
target_link_libraries(${PROJECT_NAME} 
            PUBLIC math
            PUBLIC library)

target_include_directories(${PROJECT_NAME} PUBLIC
							"${PROJECT_SOURCE_DIR}/include"
							)