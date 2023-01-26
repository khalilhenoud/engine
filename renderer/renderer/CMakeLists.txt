cmake_minimum_required(VERSION 3.22)

# set the project name
project(renderer VERSION 1.0)

if (NOT ENGINE_BUILD)
  add_subdirectory(../../math math)
endif()

# add the executable
add_library(${PROJECT_NAME} SHARED
			./source/platform/renderer_opengl_win32.c
			./source/renderer_opengl.c
			./include/renderer/internal/module.h)
			
target_link_libraries(${PROJECT_NAME}
						PUBLIC math
						PRIVATE opengl32
						PRIVATE glu32)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_SOURCE_DIR}/include"
							)