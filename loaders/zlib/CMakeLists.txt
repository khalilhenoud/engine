cmake_minimum_required(VERSION 3.22)

# set the project name
project(zlib VERSION 1.0)

# add the executable
add_library(${PROJECT_NAME} STATIC 
      ./source/adler32.c
      ./source/compress.c
      ./source/crc32.c
      ./source/deflate.c
      ./source/gzclose.c
      ./source/gzlib.c
      ./source/gzread.c
      ./source/gzwrite.c
      ./source/infback.c
      ./source/inffast.c
      ./source/inflate.c
      ./source/inftrees.c
      ./source/trees.c
      ./source/uncompr.c
      ./source/zutil.c
      )

target_include_directories(${PROJECT_NAME} 
                PRIVATE "${PROJECT_SOURCE_DIR}/include/zlib"
                PUBLIC  "${PROJECT_SOURCE_DIR}/include"
							)