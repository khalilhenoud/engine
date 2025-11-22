cmake_minimum_required(VERSION 3.22)

# all libraries built by this project are copied into the output directory.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# used to omit add subdirectory from the standalone dependencies.
# currently used in renderer, when generating the render test project.
set(HAS_STANDALONE_PREDECESSOR True)

project(engine)

# TODO: the submodules that the engine downloads should be a function of the
# game project and not hard coded into the engine per se. For our case that is
# not an issue right now but should be addressed in the future. The submodules
# should be specified externally to the game (in an xml or json file per se) and
# downloaded by the engine, so the set depends on the game in question.

# DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	# Update submodules as needed
	option(GIT_SUBMODULE "Check submodules during build" ON)
	if(GIT_SUBMODULES)
		message(STATUS "Submodule update")
		execute_process(COMMAND $(GIT_EXECUTABLE) submodule update --init --recursive
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}")
		endif()
	endif()
endif()

# CHECK ALL THE SUBMODULES
if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/library/CMakeLists.txt")
	message(FATAL_ERROR "The library submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/math/CMakeLists.txt")
	message(FATAL_ERROR "The math submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/collision/CMakeLists.txt")
	message(FATAL_ERROR "The collision submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/entity/CMakeLists.txt")
	message(FATAL_ERROR "The entity submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/loaders/CMakeLists.txt")
	message(FATAL_ERROR "The loaders submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/windowing/CMakeLists.txt")
	message(FATAL_ERROR "The windowing submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/renderer/CMakeLists.txt")
	message(FATAL_ERROR "The renderer submodule was not downloaded!")
endif()

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/external/game/CMakeLists.txt")
	message(FATAL_ERROR "The game submodule was not downloaded!")
endif()

add_subdirectory(external/library library)
add_subdirectory(external/math math)
add_subdirectory(external/collision collision)
add_subdirectory(external/entity entity)
add_subdirectory(external/loaders loaders)
add_subdirectory(external/windowing windowing)
add_subdirectory(external/renderer renderer)
add_subdirectory(external/game game)

# add the executable
add_executable(${PROJECT_NAME}
			        ./source/main.c)

target_link_libraries(${PROJECT_NAME}
						  PUBLIC library
              PRIVATE game)

target_include_directories(${PROJECT_NAME} PUBLIC
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)