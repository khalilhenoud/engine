/**
 * @file collision_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef COLLISION_DATA_H
#define COLLISION_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef
enum {
  COLLIDED_FLOOR_FLAG = 1 << 0,
  COLLIDED_CEILING_FLAG = 1 << 1,
  COLLIDED_WALLS_FLAG = 1 << 2,
  COLLIDED_NONE = 1 << 3
} collision_flags_t;

typedef
struct {
  float time;
  collision_flags_t flags;
  uint32_t bvh_face_index;
} intersection_info_t;

typedef
struct {
  intersection_info_t hits[256];
  uint32_t count;
} intersection_data_t;

#ifdef __cplusplus
}
#endif

#endif