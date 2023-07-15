/**
 * @file serializer_bin.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SERIALIZER_BIN_H
#define SERIALIZER_BIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <library/allocator/allocator.h>
#include <serializer/internal/module.h>
#include <serializer/serializer_scene_data.h>


SERIALIZER_API
void
serialize_bin(
  const char* path, 
  serializer_scene_data_t* scene);

SERIALIZER_API
serializer_scene_data_t*
deserialize_bin(
  const char* path, 
  const allocator_t* allocator);

SERIALIZER_API
void
free_bin(
  serializer_scene_data_t* scene, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif