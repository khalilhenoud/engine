/**
 * @file capsule.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CAPSULE_COLLISION
#define CAPSULE_COLLISION

#ifdef __cplusplus
extern "C" {
#endif

#include <collision/internal/module.h>
#include <collision/segment_classification.h>
#include <math/c/vector3f.h>


typedef struct segment_t segment_t;
typedef struct sphere_t sphere_t;

// direction is (0, 1, 0) in all cases, a transform is required to modify this.
// NOTE: make a version of these functions that take transform into account;
typedef
struct capsule_t {
  point3f center;
  float half_height; 
  float radius;
} capsule_t;

typedef
enum sphere_capsule_classification_t {
  SPHERE_CENTER_WITHIN_CAPSULE_SEGMENT,
  SPHERE_CAPSULE_INTERSECTS,
  SPHERE_CAPSULE_DISTINCT,
  SPHERE_CAPSULE_COUNT,
} sphere_capsule_classification_t;

COLLISION_API
sphere_capsule_classification_t
classify_sphere_capsule(
  const sphere_t *source, 
  const capsule_t *capsule,
  vector3f* penetration);

COLLISION_API
segments_classification_t
classify_capsules_segments(
  const capsule_t *source, 
  const capsule_t *target, 
  segment_t *result);

typedef 
enum capsules_classification_t {
  CAPSULES_AXIS_IDENTICAL,
  CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP,
  CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP_AT_POINT,
  CAPSULES_AXIS_COLINEAR_FULL_OVERLAP,
  CAPSULES_AXIS_COPLANAR_AND_INTERSECTS,
  CAPSULES_DISTINCT,  // no distincation between parallel or not.
  CAPSULES_COLLIDE,
  CAPSULES_COUNT
} capsules_classification_t;

COLLISION_API
capsules_classification_t
classify_capsules(
  const capsule_t *source, 
  const capsule_t *target, 
  vector3f* penetration);

// TODO: Provide a variant with transform.
COLLISION_API
void
get_capsule_segment(
  const capsule_t *source,
  segment_t *segment);

// TODO: Provide a variant with transform.
COLLISION_API
void
get_capsule_segment_loose(
  const capsule_t *source,
  point3f *a,
  point3f *b);

#ifdef __cplusplus
}
#endif

#endif