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
  float height;
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