cmake_minimum_required(VERSION 3.22)

# all libraries built by this project are copied into the output directory.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# used to omit add subdirectory from the standalone dependencies.
# currently used in renderer, when generating the render test project.
set(ENGINE_BUILD True)

project(engine)

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
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/math/CMakeLists.txt")
	message(FATAL_ERROR "The math submdule was not downloaded!")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/library/CMakeLists.txt")
	message(FATAL_ERROR "The library submdule was not downloaded!")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/entity/CMakeLists.txt")
	message(FATAL_ERROR "The entity submdule was not downloaded!")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/loaders/CMakeLists.txt")
	message(FATAL_ERROR "The loaders submdule was not downloaded!")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/renderer/CMakeLists.txt")
	message(FATAL_ERROR "The renderer submdule was not downloaded!")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/serializer/CMakeLists.txt")
	message(FATAL_ERROR "The serializer submdule was not downloaded!")
endif()

add_subdirectory(external/library library)
add_subdirectory(external/math math)
add_subdirectory(external/entity entity)
add_subdirectory(external/loaders/loaders loaders)
add_subdirectory(external/renderer/renderer renderer)
add_subdirectory(external/serializer/serializer serializer)
add_subdirectory(application)