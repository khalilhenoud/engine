/**
 * @file collision_utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-08-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <assert.h>
#include <math/c/face.h>
#include <math/c/capsule.h>
#include <entity/c/mesh/color.h>
#include <entity/c/spatial/bvh.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <application/game/logic/collision_utils.h>
#include <application/game/debug/face.h>
#include <application/game/debug/flags.h>

#define FLOOR_ANGLE_DEGREES 60


uint32_t 
is_floor(bvh_t* bvh, uint32_t index)
{
  float cosine_target = cosf(TO_RADIANS(FLOOR_ANGLE_DEGREES));
  float normal_dot = bvh->normals[index].data[1];
  return normal_dot > cosine_target;
}

uint32_t 
is_ceiling(bvh_t* bvh, uint32_t index)
{
  float cosine_target = cosf(TO_RADIANS(FLOOR_ANGLE_DEGREES));
  float normal_dot = bvh->normals[index].data[1];
  return normal_dot < -cosine_target;
}

void
populate_capsule_aabb(
  bvh_aabb_t* aabb, 
  const capsule_t* capsule, 
  const float multiplier)
{
  aabb->min_max[0] = capsule->center;
  aabb->min_max[1] = capsule->center;

  {
    vector3f min;
    vector3f_set_3f(
      &min,
      -capsule->radius * multiplier,
      (-capsule->half_height - capsule->radius) * multiplier,
      -capsule->radius * multiplier);
    add_set_v3f(aabb->min_max, &min);
    mult_set_v3f(&min, -1.f);
    add_set_v3f(aabb->min_max + 1, &min);
  }
}

void
populate_moving_capsule_aabb(
  bvh_aabb_t* aabb, 
  const capsule_t* capsule, 
  const vector3f* displacement, 
  const float multiplier)
{
  bvh_aabb_t start_end[2];
  capsule_t copy = *capsule;
  populate_capsule_aabb(start_end + 0, &copy, multiplier);
  add_set_v3f(&copy.center, displacement);
  populate_capsule_aabb(start_end + 1, &copy, multiplier);
  merge_aabb(aabb, start_end + 0, start_end + 1);
}

int32_t
is_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t array[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, array, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t *node = bvh->nodes + array[used_index];
    for (
      uint32_t i = node->left_first, last = node->left_first + node->tri_count; 
      i < last; ++i) {
      if (!bounds_intersect(&bounds, bvh->bounds + i))
        continue;

      classification =
        classify_capsule_face(
          capsule,
          bvh->faces + i,
          bvh->normals + i,
          0,
          &penetration,
          &sphere_center);

      length_sqrd = length_squared_v3f(&penetration);

      if (
        classification != CAPSULE_FACE_NO_COLLISION && 
        !IS_ZERO_LP(length_sqrd))
        return 0;
    }
  }

  return 1;
}

void
ensure_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t array[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, array, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t* node = bvh->nodes + array[used_index];
    for (
      uint32_t i = node->left_first, last = node->left_first + node->tri_count; 
      i < last; ++i) {

      if (!bounds_intersect(&bounds, bvh->bounds + i))
        continue;

      classification =
        classify_capsule_face(
          capsule,
          bvh->faces + i,
          bvh->normals + i,
          0,
          &penetration,
          &sphere_center);

      if (classification != CAPSULE_FACE_NO_COLLISION)
        add_set_v3f(&capsule->center, &penetration);
    }
  }
}

int32_t
intersects_post_displacement(
  capsule_t capsule,
  const vector3f displacement,
  const face_t* face,
  const vector3f* normal,
  const uint32_t iterations,
  const float limit_distance);

uint32_t
get_time_of_impact(
  bvh_t *bvh,
  capsule_t *capsule,
  vector3f displacement,
  intersection_info_t collision_info[256],
  const uint32_t iterations,
  const float limit_distance)
{
  uint32_t array[256];
  uint32_t used = 0;
  uint32_t collision_info_used = 0;
  bvh_aabb_t bounds;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };

  // initialize the first element.
  collision_info[0] = info;

  populate_moving_capsule_aabb(&bounds, capsule, &displacement, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, array, &used);

  if (used && g_debug_flags.draw_collision_query) {
    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (
        uint32_t i = node->left_first, 
        last = node->left_first + node->tri_count; 
        i < last; ++i) {

        {
          debug_color_t color = 
            is_floor(bvh, i) ? green : (is_ceiling(bvh, i) ? white : blue);
          int32_t thickness = is_floor(bvh, i) ? 3 : 2;
          add_debug_face_to_frame(
            bvh->faces + i, bvh->normals + i, color, thickness);
        }
      }
    }
  }

  if (used) {
    float time;

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (
        uint32_t i = node->left_first, 
        last = node->left_first + node->tri_count; 
        i < last; ++i) {

        if (!bounds_intersect(&bounds, bvh->bounds + i))
          continue;

        // Ignore any face that does not intersect post displacement, that is a
        // problem for continuous collision detection.
        if (
          !intersects_post_displacement(
            *capsule,
            displacement,
            bvh->faces + i,
            bvh->normals + i,
            iterations,
            limit_distance))
          continue;      

        time = find_capsule_face_intersection_time(
          *capsule,
          bvh->faces + i,
          bvh->normals + i,
          displacement,
          iterations,
          limit_distance);

        if (
          time < collision_info[0].time || 
          IS_SAME_MP(time, collision_info[0].time)) {
          collision_info_used = 
            time < collision_info[0].time ? 0 : collision_info_used;
            
          collision_info[collision_info_used].time = time;
          collision_info[collision_info_used].flags = COLLIDED_NONE;

          collision_info[collision_info_used].flags = 
            is_floor(bvh, i) ? 
            COLLIDED_FLOOR_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].flags = 
            is_ceiling(bvh, i) ? 
            COLLIDED_CEILING_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].flags = 
            collision_info[collision_info_used].flags == COLLIDED_NONE ? 
            COLLIDED_WALLS_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].bvh_face_index = i;
          collision_info_used++;
          assert(collision_info_used < 256);
        }
      }
    }
  }

  return collision_info_used;
}

static
int32_t
intersects_post_displacement(
  capsule_t capsule,
  const vector3f displacement,
  const face_t* face,
  const vector3f* normal,
  const uint32_t iterations,
  const float limit_distance)
{
  vector3f penetration;
  point3f sphere_center;
  capsule_face_classification_t classification;

  add_set_v3f(&capsule.center, &displacement);
  classification = classify_capsule_face(
    &capsule, face, normal, 0, &penetration, &sphere_center);
  return classification != CAPSULE_FACE_NO_COLLISION;
}