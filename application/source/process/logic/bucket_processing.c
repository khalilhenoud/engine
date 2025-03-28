/**
 * @file bucket_processing.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <assert.h>
#include <math/c/face.h>
#include <math/c/capsule.h>
#include <entity/c/mesh/color.h>
#include <entity/c/spatial/bvh.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <application/process/logic/bucket_processing.h>


extern int32_t draw_ignored_faces;
extern int32_t disable_depth_debug;
extern int32_t use_locked_motion;
extern int32_t draw_collision_query;
extern int32_t draw_collided_face;
extern int32_t draw_status;

extern color_t red;
extern color_t green;
extern color_t blue;
extern color_t yellow;
extern color_t white;
extern color_t gen;

// return 0 if not split, 1 if split.
static
int32_t
classify_buckets(
  bvh_t* bvh, 
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256],
  uint32_t bucket_count, 
  collision_flags_t flag,
  uint32_t bucket_index,
  uint32_t excluded_index)
{
  face_t* face0,* face1;
  vector3f* normal0;
  uint32_t index0, index1;
  point_halfspace_classification_t classify[3];
  uint32_t faces_in_front, faces_to_back;

  assert(bucket_count);
  
  for (uint32_t i = 0; i < bucket_count; ++i) {
    // these are the buckets that form colinear opposite walls
    if (i == bucket_index || i == excluded_index)
      continue;

    // again limit this to similar specs
    if ((collision_info[bucket_offset[i]].flags & flag) == 0)
      continue;

    // this is the plane, test the original bucket faces to check if they lie on 
    // both sides of the plane. 
    index0 = collision_info[bucket_offset[i]].bvh_face_index;
    face0 = bvh->faces + index0;
    normal0 = bvh->normals + index0;
    faces_in_front = faces_to_back = 0;

    for (uint32_t j = 0; j < buckets[bucket_index]; ++j) {
      uint32_t points_in_front = 0, points_to_back = 0;
      index1 = collision_info[bucket_offset[bucket_index] + j].bvh_face_index;
      face1 = bvh->faces + index1;

      classify[0] = classify_point_halfspace(face0, normal0, face1->points + 0);
      classify[1] = classify_point_halfspace(face0, normal0, face1->points + 1);
      classify[2] = classify_point_halfspace(face0, normal0, face1->points + 2);
      
      points_in_front += (classify[0] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[1] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[2] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      
      points_to_back += (classify[0] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[1] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[2] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      
      // early out, meaning we have found a face that intersects the clip plane.
      if (points_in_front && points_to_back)
        return 1;
      else if (points_to_back) 
        faces_to_back++;
      else
        faces_in_front++;

      // early out if the classification contradicts the one-sidedness 
      // established prior.
      if (faces_in_front && faces_to_back)
        return 1;
    }
  }

  return 0;
}

// NOTE: the function modifies buckets[], bucket_offset[].
// TODO: make this more robust, it should also modify bucket_count and tag with
// the appropriate access specfiers to indicate intent (const etc...)
static
uint32_t
remove_bucket(
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256], 
  uint32_t bucket_count,
  uint32_t bucket_index)
{
  // remove all the faces belonging to the bucket, remove the bucket and rebuild
  // the offset.
  uint32_t start_index = bucket_offset[bucket_index];
  uint32_t end_index = start_index + buckets[bucket_index];
  uint32_t counts_affected = buckets[bucket_index];

  intersection_info_t new_info[256];
  uint32_t new_buckets[256];
  uint32_t new_bucket_offset[256];
  uint32_t new_bucket_count = bucket_count - 1;

  assert(bucket_count);

  for (uint32_t i = 0, index = 0; i < bucket_count; ++i) {
    if (i == bucket_index)
      continue;
    new_buckets[index++] = buckets[i];
  }

  for (uint32_t i = 0, index = 0; i < new_bucket_count; ++i) {
    new_bucket_offset[i] = index;
    index += new_buckets[i];
  }

  for (uint32_t i = 0, index = 0; i < info_used; ++i) {
    if (i >= start_index && i < end_index) {
      if (draw_ignored_faces)
        add_face_to_render(collision_info[i].bvh_face_index, yellow, 2);
      continue;
    }
    new_info[index++] = collision_info[i];
  }

  memcpy(buckets, new_buckets, sizeof(new_buckets));
  memcpy(bucket_offset, new_bucket_offset, sizeof(new_bucket_offset));
  memcpy(collision_info, new_info, sizeof(new_info));

  return info_used - counts_affected;
}

static
int32_t
process_buckets(
  bvh_t* const bvh, 
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256],
  uint32_t bucket_count)
{
  // processing is separated based on walls, floors, ceilings
  collision_flags_t flags[] = { 
    COLLIDED_WALLS_FLAG, 
    COLLIDED_FLOOR_FLAG | COLLIDED_CEILING_FLAG};

  // transform to total offset, instead of count.
  uint32_t bucket_offset[256] = { 0 };
  for (uint32_t i = 0, index = 0; i < bucket_count; ++i) {
    bucket_offset[i] = index;
    index += buckets[i];
  }

  assert(bucket_count);

  for (uint32_t flag_index = 0; flag_index < 2; ++flag_index) {
    planes_classification_t result;
    int32_t parters[] = { -1, -1 };
    int32_t found = 1;
    face_t* face0, * face1;
    vector3f* normal0, * normal1;
    uint32_t index0, index1;
    collision_flags_t flag = flags[flag_index];

    // while colinear opposite face buckets exist, continue.
    while (found) {
      found = 0;

      for (uint32_t i = 0, count = bucket_count - 1; i < count; ++i) {
        // only consider buckets that share the flag spec
        if ((collision_info[bucket_offset[i]].flags & flag) == 0)
          continue;
        
        index0 = collision_info[bucket_offset[i]].bvh_face_index;
        face0 = bvh->faces + index0;
        normal0 = bvh->normals + index0;

        for (uint32_t j = i + 1; j < bucket_count; ++j) {
          // again limit it to the same spec
          if ((collision_info[bucket_offset[j]].flags & flag) == 0)
            continue;

          index1 = collision_info[bucket_offset[j]].bvh_face_index;
          face1 = bvh->faces + index1;
          normal1 = bvh->normals + index1;

          result = classify_planes(face0, normal0, face1, normal1);
          if (result == PLANES_COLINEAR_OPPOSITE_FACING) {
            parters[0] = i;
            parters[1] = j; 
            found = 1;
            break;
          }
        }

        if (found)
          break;
      }

      if (found) {
        // one or both of the parters buckets will be removed.
        if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[0], parters[1])) {
          // remove parters[1]
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[1]);
          --bucket_count;
        } else if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[1], parters[0])) {
          // remove parters[0]
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[0]);
          --bucket_count;
        } else {
          // remove parters[0, 1].
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[0]);
          --bucket_count;
          --parters[1];
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[1]);
          --bucket_count;
        }
      }
    }
  }

  return info_used;
}

// this will sort collision_info with colinear faces being consecutive, the
// buckets array will contain the count per face colinearity. we return the
// bucket type.
static
uint32_t
sort_in_buckets(
  bvh_t* const bvh,
  intersection_info_t collision_info[256], 
  uint32_t info_used,
  uint32_t buckets[256])
{
  uint32_t bucket_count = 0;
  intersection_info_t sorted[256];
  uint32_t sorted_index = 0;
  uint32_t copy_info_used = info_used;
  face_t* face_a, *face_b;
  vector3f* normal_a, *normal_b;
  int64_t i, to_swap;
  intersection_info_t copy;
  planes_classification_t classify;

  while (info_used--) {
    face_a = bvh->faces + collision_info[info_used].bvh_face_index; 
    normal_a = bvh->normals + collision_info[info_used].bvh_face_index;
    
    // copy into the first sorted index.
    sorted[sorted_index++] = collision_info[info_used];
    buckets[bucket_count++] = 1;

    i = to_swap = (int64_t)info_used - 1;
    for (; i >= 0; --i) {
      face_b = bvh->faces + collision_info[i].bvh_face_index;
      normal_b = bvh->normals + collision_info[i].bvh_face_index;

      // if colinear copy into the first sorted index, swap it with to_swap 
      // index and decrease info_used.
      classify = classify_planes(face_a, normal_a, face_b, normal_b);
      if (classify == PLANES_COLINEAR) {
        sorted[sorted_index++] = collision_info[i];
        buckets[bucket_count - 1]++;

        // swap the colinear with the non-colinear.
        copy = collision_info[to_swap];
        collision_info[to_swap] = collision_info[i];
        collision_info[i] = copy;

        to_swap--;
        info_used--;
      }
    }
  }

  assert(sorted_index == copy_info_used);
  
  // copy the sorted data back.
  memcpy(collision_info, sorted, sizeof(sorted));

  return bucket_count;
}

vector3f
get_averaged_normal(
  bvh_t* const bvh,
  const uint32_t on_solid_floor,
  intersection_info_t collision_info[256],
  uint32_t info_used,
  collision_flags_t* flags)
{
  uint32_t buckets[256];
  uint32_t bucket_count = 0;

  vector3f averaged, normal;
  vector3f_set_1f(&averaged, 0.f);

  // we shouldn't consider colinear faces more than once, we need to sort this.
  bucket_count = sort_in_buckets(bvh, collision_info, info_used, buckets);

  for (uint32_t i = 0, index = 0, face_i, is_wall; i < bucket_count; ++i) {
    is_wall = 0;
    face_i = collision_info[index].bvh_face_index;
    normal = bvh->normals[face_i];

    if (is_floor(bvh, face_i))
      *flags |= COLLIDED_FLOOR_FLAG;
    else if (is_ceiling(bvh, face_i))
      *flags |= COLLIDED_CEILING_FLAG;
    else {
      *flags |= COLLIDED_WALLS_FLAG;
      is_wall = 1;
    }

    // adjust the normal, even if sloped, walls cannot contribute to vertical
    // acceleration.
    if (is_wall && on_solid_floor) {
      vector3f y_up, perp;
      vector3f_set_3f(&y_up, 0.f, 1.f, 0.f);
      perp = cross_product_v3f(&y_up, &normal);
      normalize_set_v3f(&perp);
      normal = cross_product_v3f(&perp, &y_up);
      normalize_set_v3f(&normal);
    }

    add_set_v3f(&averaged, &normal);

     if (draw_collided_face) {
       for (uint32_t k = index; k < (index + buckets[i]); ++k) {
         uint32_t d_face_i = collision_info[k].bvh_face_index;
         color_t color =
           is_floor(bvh, d_face_i) ? green :
           (is_ceiling(bvh, d_face_i) ? white : gen);
         add_face_to_render(d_face_i, color, 2);
       }
     }

    index += buckets[i];
  }

  normalize_set_v3f(&averaged);
  return averaged;
}

static
uint32_t
trim_backfacing(
  bvh_t* const bvh,
  const vector3f* velocity, 
  intersection_info_t collision_info[256],
  uint32_t info_used)
{
  intersection_info_t new_collision_info[256];
  uint32_t count = 0;
  vector3f* normal = NULL;

  for (uint32_t i = 0, index = 0; i < info_used; ++i) {
    index = collision_info[i].bvh_face_index;
    normal = bvh->normals + index;
    if (dot_product_v3f(velocity, normal) > EPSILON_FLOAT_LOW_PRECISION)
      continue;
    else
      new_collision_info[count++] = collision_info[i];
  }

  // copy the new info into the old one and return the updated info_used
  memcpy(collision_info, new_collision_info, sizeof(new_collision_info));
  return count;
}

// Note: this function does modify collision_info.
int32_t 
process_collision_info(
  bvh_t* const bvh, 
  const vector3f* velocity, 
  intersection_info_t collision_info[256], 
  int32_t info_used)
{
  // sort and process the buckets in a way the collision info used.
  if (info_used) {
    uint32_t buckets[256];
    uint32_t bucket_count = 0;
    bucket_count = sort_in_buckets(bvh, collision_info, info_used, buckets);
    info_used = process_buckets(
      bvh, collision_info, info_used, buckets, bucket_count);
  }

  return trim_backfacing(bvh, velocity, collision_info, info_used);
}