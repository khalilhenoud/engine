/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SERIALIZER_UTILS_INCLUDED_H
#define SERIALIZER_UTILS_INCLUDED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <serializer/internal/module.h>


typedef uintptr_t file_handle_t;

typedef 
enum file_open_flags_t {
  FILE_OPEN_MODE_INVALID = 1,
  FILE_OPEN_MODE_READ = 1 << 1,
  FILE_OPEN_MODE_WRITE = 1 << 2,
  FILE_OPEN_MODE_WRITE_NO_OVERWRITE = 1 << 3,
  FILE_OPEN_MODE_APPEND = 1 << 4,
  FILE_OPEN_MODE_UPDATE = 1 << 5,
  FILE_OPEN_MODE_BINARY = 1 << 6,
  FILE_OPEN_MODE_COUNT = 1 << 7
} file_open_flags_t;

SERIALIZER_API
file_handle_t
open_file(
  const char* path,
  file_open_flags_t flags);

SERIALIZER_API
size_t
write_buffer(
  file_handle_t file,
  const void* data,
  size_t elem_size,
  size_t count);

SERIALIZER_API
size_t
read_buffer(
  file_handle_t file,
  void* data,
  size_t elem_size,
  size_t count);

SERIALIZER_API
void
close_file(file_handle_t file);


#ifdef __cplusplus
}
#endif

#endif