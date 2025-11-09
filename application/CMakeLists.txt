cmake_minimum_required(VERSION 3.22)

# set the project name
project(application VERSION 1.0)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME}
      ./source/game/memory_tracking/memory_tracking.cpp
      ./source/game/rendering/load_font.c
      ./source/game/rendering/load_image.c
      ./source/game/rendering/render_data.c
      ./source/game/logic/player.c
      ./source/game/logic/camera.c
      ./source/game/logic/collision_utils.c
      ./source/game/logic/bucket_processing.c
      ./source/game/debug/color.c
      ./source/game/debug/text.c
      ./source/game/debug/face.c
      ./source/game/debug/flags.c
      ./source/game/levels/generic_level.c
      ./source/game/levels/room_select.c
      ./source/game/levels/load_scene.c
      ./source/game/input/platform/input_platform_win32.c
      ./source/game/input/input.c
      ./source/game/game.c
			./source/main.cpp)

target_link_libraries(${PROJECT_NAME}
						PUBLIC library
						PUBLIC math
            PUBLIC collision
            PRIVATE windowing
						PRIVATE entity
						PRIVATE loaders
						PRIVATE renderer)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)