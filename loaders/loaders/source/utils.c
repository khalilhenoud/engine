/**
 * @file utils.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <stdio.h>
#include <loaders/utils.h>


const char*
read_file_as_ascii(
  const char* path, 
  const allocator_t* allocator, 
  size_t* file_size)
{
  char* data = NULL;
  FILE* file = fopen(path, "rb");
  if (!file)
    return data;

  fseek(file, 0L, SEEK_END);
  *file_size = ftell(file);
  rewind(file);
  
  data = (char *)allocator->mem_alloc(*file_size);
  fread(data, *file_size, 1, file);
  fclose(file);

	return data;
}