cmake_minimum_required(VERSION 3.22)

# set the project name
project(library VERSION 1.0)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${PROJECT_SOURCE_DIR}/include")