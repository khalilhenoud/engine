/**
 * @file utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <serializer/utils.h>


file_handle_t
open_file(
  const char* path,
  file_open_flags_t flags)
{
  FILE* file = NULL;
  char cstr_flags[8];
  int32_t read = (flags & FILE_OPEN_MODE_READ) / FILE_OPEN_MODE_READ;
  int32_t write = (flags & FILE_OPEN_MODE_WRITE) / FILE_OPEN_MODE_WRITE;
  int32_t write_no_overwrite = (flags & FILE_OPEN_MODE_WRITE_NO_OVERWRITE) / FILE_OPEN_MODE_WRITE_NO_OVERWRITE;
  int32_t append = (flags & FILE_OPEN_MODE_APPEND) / FILE_OPEN_MODE_APPEND;

  assert(path != NULL && "path cannot be null");
  assert(!(flags & FILE_OPEN_MODE_INVALID) && "file open invalid!!");
  assert(
    (read + write + write_no_overwrite + append) == 1 &&
    "Do not combine read/write/writenooverwrite/append flags! Choose one!");

  memset(cstr_flags, 0, sizeof(cstr_flags));
  if (read)
    sprintf(cstr_flags, "r");
  else if (write || write_no_overwrite)
    sprintf(cstr_flags, "w");
  else if (append)
    sprintf(cstr_flags, "a");

  if (flags & FILE_OPEN_MODE_BINARY)
    sprintf(cstr_flags + strlen(cstr_flags), "b");

  if (flags & FILE_OPEN_MODE_UPDATE)
    sprintf(cstr_flags + strlen(cstr_flags), "+");

  if (write_no_overwrite)
    sprintf(cstr_flags + strlen(cstr_flags), "x");

  file = fopen(path, cstr_flags);
  return (file_handle_t)file;
}

size_t
write_buffer(
  file_handle_t file,
  const void* data,
  size_t elem_size,
  size_t count)
{
  assert((FILE *)file != NULL);
  return fwrite(data, elem_size, count, (FILE *)file);
}

size_t
read_buffer(
  file_handle_t file,
  void* data,
  size_t elem_size,
  size_t count)
{
  assert((FILE *)file != NULL);
  return fread(data, elem_size, count, (FILE *)file);
}

SERIALIZER_API
void
close_file(file_handle_t file)
{
  assert((FILE *)file != NULL);
  fclose((FILE *)file);
}