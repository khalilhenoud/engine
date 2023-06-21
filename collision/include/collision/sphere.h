/**
 * @file sphere.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SPHERE_COLLISION
#define SPHERE_COLLISION

#ifdef __cplusplus 
extern "C" {
#endif

#include <collision/internal/module.h>
#include <math/c/vector3f.h>


typedef
struct sphere_t {
  point3f center;
  float radius;
} sphere_t;

typedef
enum spheres_classification_t {
  SPHERES_DISTINCT,
  SPHERES_COLLIDE,
  SPHERES_SHARE_CENTER,
  SPHERES_IDENTICAL,
  SPHERES_COUNT
} spheres_classification_t;

COLLISION_API
spheres_classification_t
classify_spheres(
  const sphere_t *source, 
  const sphere_t *target,
  vector3f* penetration);


#ifdef __cplusplus
}
#endif

#endif