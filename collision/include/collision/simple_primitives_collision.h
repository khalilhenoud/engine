/**
 * @file basic_shapes_collision.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef BASIC_SHAPES_COLLISION
#define BASIC_SHAPES_COLLISION

#ifdef __cplusplus
extern "C" {
#endif

#include <collision/internal/module.h>
#include <collision/simple_primitives.h>

/**
 * sphere transform
 *  position
 *  uniform scale
 * 
 * capsule transform
 *  position
 *  rotation
 *  scale uniform
 * 
 * face(s) transform
 *  position
 *  rotation
 *  non-uniform scale (these are vqs for all intent and purposes).
 */

// TODO: Provide a transform variant of these apis
COLLISION_API
collision_result_t
collision_spheres(const sphere_t *source, const sphere_t *target);

COLLISION_API
collision_result_t
collision_sphere_face(const sphere_t *source, const face_t *face);

COLLISION_API
collision_result_t
collision_sphere_faces(const sphere_t* source, const faces_t* face);
 
COLLISION_API
collision_result_t
collision_sphere_capsule(const sphere_t *source, const capsule_t *capsule);

COLLISION_API
collision_result_t
collision_capsules(const capsule_t *source, const capsule_t *target);

COLLISION_API
collision_result_t
collision_capsule_face(const capsule_t *source, const face_t *face);

COLLISION_API
primitives_classification_t
classify_segments(
  const segment_t *first, 
  const segment_t *second, 
  segment_t *out);

#ifdef __cplusplus
}
#endif

#endif