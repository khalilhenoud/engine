/**
 * @file bvh.h
 * @author khalilhenoud@gmail.com
 * @brief non-general bvh for simplistic collision detection.
 * @version 0.1
 * @date 2023-10-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SPATIAL_BVH_H
#define SPATIAL_BVH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math/c/vector3f.h>


typedef struct allocator_t allocator_t;

// TODO(khalil): major improvements could be done to this code for now it is
// fine as is, but a better implementation could be very hepful down the road.

// TODO: This is useful, should not be package specific.
typedef
struct bvh_aabb_t {
  point3f min_max[2];
} bvh_aabb_t;

typedef
struct bvh_face_t {
  point3f points[3];
  point3f centroid;
  vector3f normal;
  bvh_aabb_t aabb;
  uint8_t is_valid;
  // TODO: These are optional, move to meta arrays.
  float normal_dot;
  uint8_t is_floor;
  uint8_t is_ceiling;
} bvh_face_t;

typedef
struct bvh_node_t {
  bvh_aabb_t bounds;
  // TODO: right index is always left index + 1, remove it.
  uint32_t left_index, right_index;
  uint32_t first_prim, last_prim;
} bvh_node_t;

typedef
struct bvh_t {
  bvh_face_t* faces;
  uint32_t count;
  bvh_node_t* nodes;
  uint32_t nodes_count;
  uint32_t nodes_used;
} bvh_t;

typedef
enum bvh_construction_method_t {
  BVH_CONSTRUCT_NAIVE,
  BVH_CONSTRUCT_COUNT
} bvh_construction_method_t;

// takes a list of indices and vertices...
bvh_t*
create_bvh(
  float** vertices, 
  uint32_t** indices, 
  uint32_t* indices_count, 
  uint32_t meshes_count,
  const allocator_t* allocator,
  bvh_construction_method_t method);

void
free_bvh(bvh_t* bvh, const allocator_t* allocator);

uint32_t
get_bvh_primitives_per_leaf(void);

int32_t
bounds_intersect(const bvh_aabb_t* left, const bvh_aabb_t* right);

// NOTE: A max 256 leaf indices returned, if used is 0 no intersection.
void
query_intersection(
  bvh_t* bvh, 
  bvh_aabb_t* bound, 
  uint32_t array[256], 
  uint32_t* used);

#ifdef __cplusplus
}
#endif

#endif