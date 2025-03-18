/**
 * @file player_collision_utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-08-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef PLAYER_COLLISION_UTILS_H
#define PLAYER_COLLISION_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math/c/vector3f.h>


typedef struct face_t face_t;
typedef struct bvh_t bvh_t;
typedef struct bvh_aabb_t bvh_aabb_t;
typedef struct capsule_t capsule_t;
typedef struct pipeline_t pipeline_t;

void
draw_collision_debug_data(
  bvh_t* bvh,
  pipeline_t* pipeline,
  const int32_t disable_depth);

void
add_face_to_render(
  uint32_t index, 
  color_t color, 
  int32_t thickness);

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

uint32_t 
is_floor(bvh_t* bvh, uint32_t index);

uint32_t 
is_ceiling(bvh_t* bvh, uint32_t index);

void
populate_capsule_aabb(
  bvh_aabb_t* aabb, 
  const capsule_t* capsule, 
  const float multiplier);

void
populate_moving_capsule_aabb(
  bvh_aabb_t* aabb, 
  const capsule_t* capsule, 
  const vector3f* displacement, 
  const float multiplier);

uint32_t
get_all_first_time_of_impact(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  intersection_info_t collision_info[256],
  const uint32_t iterations,
  const float limit_distance,
  const int32_t draw_collision_query);

int32_t
is_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule);

void
ensure_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule);

#ifdef __cplusplus
}
#endif

#endif