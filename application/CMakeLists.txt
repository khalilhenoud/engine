cmake_minimum_required(VERSION 3.22)

# set the project name
project(application VERSION 1.0)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} WIN32 
			./source/application.cpp
			./source/main.cpp
      ./source/input.c
      ./source/platform/input_platform_win32.c)
			
target_link_libraries(${PROJECT_NAME}
						PUBLIC math
            PRIVATE entity
						PRIVATE loaders
						PRIVATE renderer
						PRIVATE winmm)

# TODO: add the math library
target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)