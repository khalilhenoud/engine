cmake_minimum_required(VERSION 3.22)

# set the project name
project(application VERSION 1.0)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} WIN32
      ./source/converters/to_render_data.c
      ./source/converters/csv_to_font.c
      ./source/converters/png_to_image.c
      ./source/process/render_data/utils.c
      ./source/process/logic/player.c
      ./source/process/logic/player_collision_utils.c
      ./source/process/logic/bucket_processing.c
      ./source/process/debug/color.c
      ./source/process/debug/text.c
      ./source/process/debug/face.c
      ./source/process/levels/generic_level.c
      ./source/process/levels/room_select.c
      ./source/process/levels/load_scene.c
			./source/application.cpp
			./source/main.cpp
      ./source/input.c
      ./source/platform/input_platform_win32.c)
			
target_link_libraries(${PROJECT_NAME}
						PUBLIC library
						PUBLIC math
						PRIVATE entity
						PRIVATE loaders
						PRIVATE renderer
						PRIVATE winmm)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)