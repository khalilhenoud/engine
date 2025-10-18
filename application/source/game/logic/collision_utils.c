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
#include <entity/c/spatial/bvh.h>
#include <application/game/logic/collision_utils.h>
#include <application/game/debug/face.h>
#include <application/game/debug/flags.h>

#define FLOOR_ANGLE_DEGREES 60


uint32_t 
is_floor(bvh_t *bvh, uint32_t index)
{
  float cosine_target = cosf(TO_RADIANS(FLOOR_ANGLE_DEGREES));
  float normal_dot = (cvector_as(&bvh->normals, index, vector3f))->data[1];
  return normal_dot > cosine_target;
}

uint32_t 
is_ceiling(bvh_t *bvh, uint32_t index)
{
  float cosine_target = cosf(TO_RADIANS(FLOOR_ANGLE_DEGREES));
  float normal_dot = (cvector_as(&bvh->normals, index, vector3f))->data[1];
  return normal_dot < -cosine_target;
}

debug_color_t
get_debug_color(bvh_t *bvh, uint32_t index)
{
  return is_floor(bvh, index) ? green : (is_ceiling(bvh, index) ? white : blue);
}

collision_flags_t
get_collision_flag(bvh_t *bvh, uint32_t index)
{
  collision_flags_t flag = COLLIDED_NONE;
  flag = is_floor(bvh, index) ? COLLIDED_FLOOR_FLAG : flag;
  flag = is_ceiling(bvh, index) ? COLLIDED_CEILING_FLAG : flag;
  flag = (flag == COLLIDED_NONE) ? COLLIDED_WALLS_FLAG : flag;
  return flag;
}

void
populate_capsule_aabb(
  bvh_aabb_t *aabb, 
  const capsule_t *capsule, 
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
  bvh_aabb_t *aabb, 
  const capsule_t *capsule, 
  const vector3f *displacement, 
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
  bvh_t *bvh,
  capsule_t *capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t query[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, query, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t *node = cvector_as(&bvh->nodes, query[used_index], bvh_node_t);
    for (
      uint32_t i = node->left_first, last = node->left_first + node->tri_count; 
      i < last; ++i) {
      if (!bounds_intersect(&bounds, cvector_as(&bvh->bounds, i, bvh_aabb_t)))
        continue;

      classification =
        classify_capsule_face(
          capsule,
          cvector_as(&bvh->faces, i, face_t),
          cvector_as(&bvh->normals, i, vector3f),
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
  bvh_t *bvh,
  capsule_t *capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t query[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, query, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t *node = cvector_as(&bvh->nodes, query[used_index], bvh_node_t);
    for (
      uint32_t i = node->left_first, last = node->left_first + node->tri_count; 
      i < last; ++i) {

      if (!bounds_intersect(&bounds, cvector_as(&bvh->bounds, i, bvh_aabb_t)))
        continue;

      classification =
        classify_capsule_face(
          capsule,
          cvector_as(&bvh->faces, i, face_t),
          cvector_as(&bvh->normals, i, vector3f),
          0,
          &penetration,
          &sphere_center);

      if (classification != CAPSULE_FACE_NO_COLLISION)
        add_set_v3f(&capsule->center, &penetration);
    }
  }
}

inline
int32_t
intersects_post_displacement(
  capsule_t capsule,
  const vector3f displacement,
  const face_t *face,
  const vector3f *normal)
{
  vector3f penetration;
  point3f sphere_center;
  capsule_face_classification_t classification;

  add_set_v3f(&capsule.center, &displacement);
  classification = classify_capsule_face(
    &capsule, face, normal, 0, &penetration, &sphere_center);
  return classification != CAPSULE_FACE_NO_COLLISION;
}

uint32_t
get_time_of_impact(
  bvh_t *bvh,
  capsule_t *capsule,
  vector3f displacement,
  intersection_info_t collision_info[256],
  const uint32_t iterations,
  const float limit_distance)
{
  uint32_t query[256];
  uint32_t query_hits = 0;
  uint32_t hits = 0;
  bvh_aabb_t bounds;
  vector3f unit = normalize_v3f(&displacement);
  intersection_info_t *first = collision_info;

  // initialize the first element, this represents the minimum toi if any.
  first->time = 1.f;
  first->flags = COLLIDED_NONE;
  first->bvh_face_index = (uint32_t)-1;

  populate_moving_capsule_aabb(&bounds, capsule, &displacement, 1.025f);
  query_intersection_fixed_256(bvh, &bounds, query, &query_hits);

  if (query_hits && g_debug_flags.draw_collision_query) {
    uint32_t index = 0;

    for (; index < query_hits; ++index) {
      bvh_node_t *node = cvector_as(&bvh->nodes, query[index], bvh_node_t);
      uint32_t i = node->left_first;
      uint32_t last = node->left_first + node->tri_count;
      for (; i < last; ++i) {
        debug_color_t color = get_debug_color(bvh, i);
        int32_t width = is_floor(bvh, i) ? 3 : 2;
        add_debug_face_to_frame(
          cvector_as(&bvh->faces, i, face_t), 
          cvector_as(&bvh->normals, i, vector3f), 
          color, width);
      }
    }
  }

  if (query_hits) {
    float time;
    uint32_t index = 0;

    for (; index < query_hits; ++index) {
      bvh_node_t *node = cvector_as(&bvh->nodes, query[index], bvh_node_t);
      uint32_t i = node->left_first;
      uint32_t last = node->left_first + node->tri_count;
      
      for (; i < last; ++i) {
        face_t *face = cvector_as(&bvh->faces, i, face_t);
        vector3f *normal = cvector_as(&bvh->normals, i, vector3f);

        if (!bounds_intersect(&bounds, cvector_as(&bvh->bounds, i, bvh_aabb_t)))
          continue;

        // ignore any face that does not intersect post displacement, that is a
        // problem for continuous collision detection (tunneling).
        if (!intersects_post_displacement(*capsule, displacement, face, normal))
          continue;

        // NOTE: we do not ignore faces that we are to the back of, the bucket
        // processing handles that.
        time = find_capsule_face_intersection_time(
          *capsule,
          face,
          normal,
          displacement,
          iterations,
          limit_distance);

        if (time < first->time || IS_SAME_MP(time, first->time)) {
          hits = time < first->time ? 0 : hits;
          collision_info[hits].time = time;
          collision_info[hits].flags = get_collision_flag(bvh, i);
          collision_info[hits].bvh_face_index = i;
          hits++;
          assert(hits < 256);
        }
      }
    }
  }

  return hits;
}