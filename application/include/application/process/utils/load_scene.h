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


typedef struct scene_t scene_t;
typedef struct allocator_t allocator_t;

scene_t*
load_scene(
  const char* dataset, 
  const char* folder, 
  const char* file,
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif