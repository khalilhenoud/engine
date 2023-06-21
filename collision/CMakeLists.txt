cmake_minimum_required(VERSION 3.22)

# set the project name
project(collision VERSION 1.0)

# add the executable
add_library(${PROJECT_NAME} SHARED
      ./source/segment.c
      ./source/sphere.c
      ./source/capsule.c
      ./source/face.c
      ./include/collision/internal/module.h)

target_link_libraries(${PROJECT_NAME} 
      PUBLIC math)
            
target_include_directories(${PROJECT_NAME} 
      PUBLIC 
      "${PROJECT_SOURCE_DIR}/include")