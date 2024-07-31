/**
 * @file bin_to_scene.h
 * @author khalilhenoud@gmail.com
 * @brief convert a bin scene into an entity scene.
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef BIN_TO_ENTITY_H
#define BIN_TO_ENTITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <entity/c/mesh/color.h>


typedef struct serializer_scene_data_t serializer_scene_data_t;
typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;

scene_t*
bin_to_scene(
  serializer_scene_data_t* scene, 
  const allocator_t* allocator);

scene_t*
load_scene_from_bin(
  const char* dataset, 
  const char* folder, 
  const char* file,
  uint32_t override_ambient, 
  color_rgba_t ambient, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif