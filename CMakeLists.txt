cmake_minimum_required(VERSION 3.22)

# all libraries built by this project are copied into the output directory.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# used to omit add subdirectory from the standalone dependencies.
set(ENGINE_BUILD True)

project(engine)

add_subdirectory(math)
add_subdirectory(entity)
add_subdirectory(loaders/loaders)
add_subdirectory(renderer/renderer)
add_subdirectory(application)