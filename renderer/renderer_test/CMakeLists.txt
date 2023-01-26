cmake_minimum_required(VERSION 3.22)

# all libraries built by this project are copied into the output directory.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# set the project name
project(renderer_test VERSION 1.0)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(../renderer renderer)

# add the executable
add_executable(${PROJECT_NAME} WIN32
				./source/main.cpp
        ./source/application.cpp
				)
			
target_link_libraries(${PROJECT_NAME}
						PRIVATE renderer
						)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)