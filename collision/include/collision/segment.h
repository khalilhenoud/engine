/**
 * @file segment.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SEGMENT_COLLISION
#define SEGMENT_COLLISION

#ifdef __cplusplus
extern "C" {
#endif

#include <collision/internal/module.h>
#include <collision/segment_classification.h>
#include <math/c/vector3f.h>


typedef 
struct segment_t {
  point3f points[2];
} segment_t;

COLLISION_API
vector3f 
closest_point_on_segment(
  const vector3f* point, 
  const segment_t* target);

COLLISION_API
vector3f 
closest_point_on_segment_loose(
  const vector3f* point, 
  const vector3f* a,
  const vector3f* b);

// NOTE: this function guarantees that the first point in 'out' belongs to
// 'first' and vice versa (full overlap cases are the exception for obvious
// reasons).
COLLISION_API
segments_classification_t
classify_segments(
  const segment_t *first, 
  const segment_t *second, 
  segment_t *out);

#ifdef __cplusplus
}
#endif

#endif