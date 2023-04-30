cmake_minimum_required(VERSION 3.22)

# set the project name
project(loaders VERSION 1.0)

add_subdirectory(../libpng libpng)

# add the executable
add_library(${PROJECT_NAME} SHARED
			./source/loader_ase.c
			./source/loader_csv.c
			./source/loader_png.c
			./source/utils.c
			./include/loaders/internal/module.h
      )
			
target_link_libraries(${PROJECT_NAME} 
      PUBLIC library
      PRIVATE libpng)

target_include_directories(${PROJECT_NAME} PUBLIC
      "${PROJECT_SOURCE_DIR}/include"
      )