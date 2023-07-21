cmake_minimum_required(VERSION 3.22)

# set the project name
project(application VERSION 1.0)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} WIN32 
      ./source/converters/ase_to_entity.cpp
      ./source/converters/entity_to_render_data.cpp
      ./source/converters/entity_to_bin.cpp
      ./source/converters/bin_to_entity.cpp
      ./source/converters/mesh_to_render_data.cpp
      ./source/converters/png_to_image.cpp
      ./source/converters/csv_to_font.cpp
			./source/application.cpp
			./source/main.cpp
      ./source/input.c
      ./source/platform/input_platform_win32.c)
			
target_link_libraries(${PROJECT_NAME}
						PUBLIC library
						PUBLIC math
            PRIVATE collision
            PRIVATE data_format
						PRIVATE entity
						PRIVATE loaders
						PRIVATE renderer
						PRIVATE winmm
            PRIVATE serializer)

# TODO: add the math library
target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)