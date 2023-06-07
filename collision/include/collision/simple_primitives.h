/**
 * @file basic_shapes.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef BASIC_SHAPES_H
#define BASIC_SHAPES_H

#include <math/c/vector3f.h>

typedef 
struct {
  point3f points[2];
} segment_t;

// TODO: Remake, redo...

typedef
enum {
  CLASSIFIED_IDENTICAL,
  CLASSIFIED_PARALLEL,
  CLASSIFIED_COLINEAR_NO_OVERLAP,
  CLASSIFIED_COLINEAR_FULL_OVERLAP,
  CLASSIFIED_COLINEAR_OVERLAPPING,
  CLASSIFIED_COLINEAR_OVERLAPPING_AT_POINT,
  CLASSIFIED_COPLANAR_INTERSECT,
  CLASSIFIED_COPLANAR_NO_INTERSECT,
  CLASSIFIED_DISTINCT,
  CLASSIFIED_COUNT
} primitives_classification_t;

typedef 
enum {
  CAPSULES_AXIS_IDENTICAL,
  CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP,
  CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP_AT_POINT,
  CAPSULES_AXIS_COLINEAR_FULL_OVERLAP,
  CAPSULES_DISTINCT,  // no distincation between parallel or not.
  CAPSULES_COUNT
} capsules_classification_t;

// transforms should be part of these structures.
typedef
struct {
  vector3f direction;
  float penetration;
} collision_result_t;

typedef
struct {
  point3f center;
  float radius;
} sphere_t;

typedef
struct {
  point3f center;
  float half_height; // direction is (0, 1, 0) in all cases, rotation is needed to make this rotate.
  float radius;
} capsule_t;

//
// TODO: normals should be cached.
typedef
struct {
  point3f points[3];
} face_t;

// TODO: normals should be cached.
typedef
struct {
  const float* vertices;
  uint32_t vertices_count;
  const uint32_t* indices;
  uint32_t indices_count;
} faces_t;

#endif