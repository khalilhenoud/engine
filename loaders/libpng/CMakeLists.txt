cmake_minimum_required(VERSION 3.22)

# set the project name
project(libpng VERSION 1.0)

add_subdirectory(../zlib zlib)

# add the executable
add_library(${PROJECT_NAME} STATIC 
        ./source/png.c
        ./source/pngerror.c
        ./source/pngget.c
        ./source/pngmem.c
        ./source/pngpread.c
        ./source/pngread.c
        ./source/pngrio.c
        ./source/pngrtran.c
        ./source/pngrutil.c
        ./source/pngset.c
        ./source/pngtrans.c
        ./source/pngwio.c
        ./source/pngwrite.c
        ./source/pngwtran.c
        ./source/pngwutil.c
      )

target_link_libraries(${PROJECT_NAME} PUBLIC zlib)

target_include_directories(${PROJECT_NAME} 
                PRIVATE "${PROJECT_SOURCE_DIR}/include/libpng"
                PUBLIC "${PROJECT_SOURCE_DIR}/include"
							)