/**
 * @file bucket_processing.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef BUCKET_PROCESSING_H
#define BUCKET_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math/c/vector3f.h>
#include <application/game/logic/collision_data.h>


typedef struct bvh_t bvh_t;
typedef struct bvh_aabb_t bvh_aabb_t;
typedef struct capsule_t capsule_t;

collision_flags_t
get_averaged_normal_filtered(
  const vector3f *orientation,
  bvh_t *const bvh,
  vector3f *averaged,
  intersection_info_t collision_info[256],
  const uint32_t info_used,
  const uint32_t on_solid_floor,
  const collision_flags_t flags,
  const uint32_t adjust_non_walkable);

/**
 * Processes the collision information into buckets, basically in groups. 
 * This is important for not over-representing face normals in collision 
 * calculation. It also serves to cull groups that nullify each other's 
 * contributions. 'collision_info' is modified in this process, and an updated
 * 'info_used' is returned.
 */
uint32_t 
process_collision_info(
  bvh_t *const bvh,
  const vector3f *velocity,
  intersection_info_t collision_info[256],
  uint32_t info_used);

#ifdef __cplusplus
}
#endif

#endif