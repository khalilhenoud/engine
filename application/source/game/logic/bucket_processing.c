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
#include <collision/face.h>
#include <math/c/capsule.h>
#include <entity/c/spatial/bvh.h>
#include <application/game/logic/collision_utils.h>
#include <application/game/logic/bucket_processing.h>
#include <application/game/debug/color.h>
#include <application/game/debug/face.h>
#include <application/game/debug/flags.h>


/**
 * To remove a bucket (that is a part of a pair), all of the faces that belong
 * to the bucket must reside exclusively on a single side of the remaining set.
 * If both buckets currently being considered meet this criteria, we remove both
 * of them, otherwise we remove the one that isn't split. 
 * This function classify the bucket in question against the rest of the set (
 * minus its pair).
 * return 0 if not split, 1 if split.
 */
static
int32_t
classify_buckets(
  bvh_t *const bvh, 
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256],
  uint32_t bucket_count, 
  collision_flags_t flag,
  uint32_t bucket_index,
  uint32_t excluded_index)
{
  face_t *face;
  vector3f *normal;
  point_halfspace_classification_t classify[3];
  uint32_t faces_in_front, faces_to_back;

  assert(bucket_count);
  
  for (uint32_t i = 0; i < bucket_count; ++i) {
    // skip the bucket pair we are currently considering
    if (i == bucket_index || i == excluded_index)
      continue;

    // again limit to similar specs
    if ((collision_info[bucket_offset[i]].flags & flag) == 0)
      continue;

    // test the bucket faces against this plane to determine if they are split
    face = cvector_as(
      &bvh->faces, collision_info[bucket_offset[i]].bvh_face_index, face_t);
    normal = cvector_as(
      &bvh->normals, collision_info[bucket_offset[i]].bvh_face_index, vector3f);
    faces_in_front = faces_to_back = 0;

    for (uint32_t j = 0; j < buckets[bucket_index]; ++j) {
      uint32_t points_in_front = 0, points_to_back = 0;
      uint32_t face_offset = bucket_offset[bucket_index] + j;
      uint32_t bvh_index = collision_info[face_offset].bvh_face_index;
      face_t *target = cvector_as(&bvh->faces, bvh_index, face_t);

      classify[0] = classify_point_halfspace(face, normal, target->points + 0);
      classify[1] = classify_point_halfspace(face, normal, target->points + 1);
      classify[2] = classify_point_halfspace(face, normal, target->points + 2);
      
      points_in_front += (classify[0] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[1] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[2] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      
      points_to_back += (classify[0] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[1] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[2] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      
      // early out, the face is split by the plane
      if (points_in_front && points_to_back)
        return 1;
      else if (points_to_back)
        faces_to_back++;
      else
        faces_in_front++;

      // early out, the one-sidedness rule is broken
      if (faces_in_front && faces_to_back)
        return 1;
    }
  }

  return 0;
}

/**
 * Remove the bucket and all faces associated with it. Returns the remaining 
 * number of faces. collision_info, buckets and bucket_offset are all modified,
 * as is bucket_count.
 */
static
uint32_t
remove_bucket(
  bvh_t *const bvh,
  intersection_info_t collision_info[256], 
  const uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256], 
  uint32_t *const bucket_count,
  const uint32_t bucket_index)
{
  uint32_t start_index = bucket_offset[bucket_index];
  uint32_t end_index = start_index + buckets[bucket_index];
  uint32_t counts_affected = buckets[bucket_index];

  intersection_info_t new_info[256];
  uint32_t new_buckets[256];
  uint32_t new_bucket_offset[256];
  const uint32_t old_bucket_count = *bucket_count;
  const uint32_t new_bucket_count = old_bucket_count - 1;

  assert(old_bucket_count);

  for (uint32_t i = 0, index = 0; i < old_bucket_count; ++i) {
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
      if (g_debug_flags.draw_ignored_faces)
        add_debug_face_to_frame(
          cvector_as(&bvh->faces, collision_info[i].bvh_face_index, face_t), 
          cvector_as(&bvh->normals, collision_info[i].bvh_face_index, vector3f), 
          yellow, 2);
      continue;
    }
    new_info[index++] = collision_info[i];
  }

  memcpy(buckets, new_buckets, sizeof(new_buckets));
  memcpy(bucket_offset, new_bucket_offset, sizeof(new_bucket_offset));
  memcpy(collision_info, new_info, sizeof(new_info));
  --*bucket_count;
  return info_used - counts_affected;
}

/**
 * This function is responsible of trimming buckets that cancel each other out. 
 * Since we are not using CSG to remove redundant faces, we need to do that at
 * runtime. The collision detection system behaves more naturally when redundant
 * faces are removed.
 * Concident faces that cancel each other out, would either be a pair of wall 
 * faces/buckets, or a floor and a ceiling pair. 
 * Returns the remaining number of collision_info.
 */
static
uint32_t
process_buckets(
  bvh_t *const bvh,
  intersection_info_t collision_info[256],
  uint32_t info_used,
  uint32_t buckets[256],
  uint32_t bucket_count)
{
  collision_flags_t flags[] = { 
    COLLIDED_WALLS_FLAG, 
    COLLIDED_FLOOR_FLAG | COLLIDED_CEILING_FLAG};

  // build an offset buffer, a summation of the buckets face count buffer.
  uint32_t bucket_offset[256] = { 0 };
  for (uint32_t i = 0, index = 0; i < bucket_count; ++i) {
    bucket_offset[i] = index;
    index += buckets[i];
  }

  assert(bucket_count);

  for (uint32_t findex = 0; findex < 2; ++findex) {
    planes_classification_t result;
    int32_t parters[] = { -1, -1 };
    face_t *face[2];
    vector3f *normal[2];
    uint32_t index[2];
    collision_flags_t flag = flags[findex];
    int32_t found = 1;

    // while colinear opposite face buckets exist, continue processing.
    while (found) {
      found = 0;

      for (uint32_t i = 0, count = bucket_count - 1; i < count; ++i) {
        // only consider buckets that match current flag spec
        if ((collision_info[bucket_offset[i]].flags & flag) == 0)
          continue;
        
        index[0] = collision_info[bucket_offset[i]].bvh_face_index;
        face[0] = cvector_as(&bvh->faces, index[0], face_t);
        normal[0] = cvector_as(&bvh->normals, index[0], vector3f);

        for (uint32_t j = i + 1; j < bucket_count; ++j) {
          if ((collision_info[bucket_offset[j]].flags & flag) == 0)
            continue;

          index[1] = collision_info[bucket_offset[j]].bvh_face_index;
          face[1] = cvector_as(&bvh->faces, index[1], face_t);
          normal[1] = cvector_as(&bvh->normals, index[1], vector3f);

          result = classify_planes(face[0], normal[0], face[1], normal[1]);
          if (result == PLANES_COLINEAR_OPPOSITE_FACING) {
            parters[0] = (int32_t)i;
            parters[1] = (int32_t)j; 
            found = 1;
            break;
          }
        }

        if (found)
          break;
      }

      if (found) {
        // one or both of the parters buckets will be removed
        if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[0], parters[1])) {

          // remove parters[1]
          info_used = remove_bucket(
            bvh,
            collision_info, info_used, 
            buckets, bucket_offset, &bucket_count, 
            parters[1]);
        } else if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[1], parters[0])) {

          // remove parters[0]
          info_used = remove_bucket(
            bvh,
            collision_info, info_used, 
            buckets, bucket_offset, &bucket_count, 
            parters[0]);
        } else {
          // remove parters[0, 1].
          info_used = remove_bucket(
            bvh,
            collision_info, info_used, 
            buckets, bucket_offset, &bucket_count, 
            parters[0]);
          --parters[1];
          info_used = remove_bucket(
            bvh,
            collision_info, info_used, 
            buckets, bucket_offset, &bucket_count, 
            parters[1]);
        }
      }
    }
  }

  return info_used;
}

/**
 * This will sort 'collision_info' into buckets of consecutive colinear faces. 
 * 'buckets' will contain the number of consecutive faces per bucket. We return
 * the total number of buckets.
 */
static
uint32_t
sort_in_buckets(
  bvh_t *const bvh,
  intersection_info_t collision_info[256], 
  const uint32_t info_used,
  uint32_t buckets[256])
{
  uint32_t copy_info_used = info_used;
  intersection_info_t sorted[256];
  uint32_t sorted_index = 0;
  uint32_t bucket_count = 0;
  face_t *faces[2];
  vector3f *normals[2];
  int64_t i, to_swap;
  planes_classification_t classify;

  // back to front, makes no difference. cond > 0 for the loop to proceed.
  while (copy_info_used--) {
    faces[0] = cvector_as(
      &bvh->faces, collision_info[copy_info_used].bvh_face_index, face_t); 
    normals[0] = cvector_as(
      &bvh->normals, collision_info[copy_info_used].bvh_face_index, vector3f);
    
    // copy into the first sorted index, initial its number of faces.
    sorted[sorted_index++] = collision_info[copy_info_used];
    buckets[bucket_count++] = 1;

    i = to_swap = (int64_t)copy_info_used - 1;
    for (; i >= 0; --i) {
      faces[1] = cvector_as(
        &bvh->faces, collision_info[i].bvh_face_index, face_t);
      normals[1] = cvector_as(
        &bvh->normals, collision_info[i].bvh_face_index, vector3f);

      classify = classify_planes(faces[0], normals[0], faces[1], normals[1]);
      if (classify == PLANES_COLINEAR) {
        // store it next to the reference face, increase the bucket face count.
        sorted[sorted_index++] = collision_info[i];
        buckets[bucket_count - 1]++;

        {
          // swap the colinear with the non-colinear.
          intersection_info_t copy;
          copy = collision_info[to_swap];
          collision_info[to_swap] = collision_info[i];
          collision_info[i] = copy;
        }

        to_swap--;
        copy_info_used--;
      }
    }
  }

  assert(sorted_index == info_used);
  
  // copy the sorted data back, alternatively reverse the 'buckets' array since 
  // 'collision_info' is sorted in reverse order to 'sorted'.
  memcpy(collision_info, sorted, sizeof(sorted));
  return bucket_count;
}

/**
 * Get the average normal for the bucket matching the flags type. This will be
 * adjusted if we are averaging non-walkable surfaces and the option is
 * specified.
 * collision_info will be sorted in buckets, hence why it isn't const.
 * returns the aggregate flags of the faces we processed, else COLLIDED_NONE
 */
collision_flags_t
get_averaged_normal_filtered(
  const vector3f *orientation,
  bvh_t *const bvh,
  vector3f *averaged,
  intersection_info_t collision_info[256],
  const uint32_t info_used,
  const uint32_t on_solid_floor,
  const collision_flags_t flags,
  const uint32_t adjust_non_walkable)
{
  collision_flags_t return_flags = COLLIDED_NONE;
  uint32_t buckets[256];
  uint32_t bucket_count = 0;

  assert(flags != 0 && flags != COLLIDED_NONE);
  vector3f_set_1f(averaged, 0.f);

  // we sort the faces to avoid considering colinear faces more than once
  bucket_count = sort_in_buckets(bvh, collision_info, info_used, buckets);

  for (uint32_t i = 0, index = 0; i < bucket_count; index += buckets[i], ++i) {
    if ((collision_info[index].flags & flags) == 0)
      continue;

    add_set_v3f(averaged, cvector_as(
      &bvh->normals, collision_info[index].bvh_face_index, vector3f));
    return_flags |= collision_info[index].flags;

     if (g_debug_flags.draw_collided_face) {
       for (uint32_t k = index; k < (index + buckets[i]); ++k) {
         add_debug_face_to_frame(
          cvector_as(&bvh->faces, collision_info[k].bvh_face_index, face_t),
          cvector_as(&bvh->normals, collision_info[k].bvh_face_index, vector3f), 
          get_debug_color(bvh, collision_info[k].bvh_face_index), 
          2);
       }
     }
  }
  
  if (IS_ZERO_LP(length_squared_v3f(averaged)))
    return return_flags;

  normalize_set_v3f(averaged);

  // Adjust the averaged wall/ceiling normal, to behave like a vertical wall 
  // when the player is rooted. This produces better sliding motion in these 
  // cases. 
  if (on_solid_floor && adjust_non_walkable && !(flags & COLLIDED_FLOOR_FLAG)) {
    float dot;
    vector3f y_up, perp;
    vector3f_set_3f(&y_up, 0.f, 1.f, 0.f);

    // make sure we test if they are colinear.
    dot = dot_product_v3f(&y_up, averaged);
    if (IS_SAME_LP(fabs(dot), 1.f)) {
      *averaged = cross_product_v3f(averaged, orientation);
      normalize_set_v3f(averaged);
      return return_flags;
    }

    perp = cross_product_v3f(&y_up, averaged);
    normalize_set_v3f(&perp);
    *averaged = cross_product_v3f(&perp, &y_up);
    normalize_set_v3f(averaged);
  }

  return return_flags;
}

/**
 * This function has no concept of buckets, it simply removes the back facing 
 * polygons relative to velocity.
 * returns the number of collision_info remaining.
 */
static
uint32_t
trim_backfacing(
  bvh_t *const bvh,
  const vector3f *velocity, 
  intersection_info_t collision_info[256],
  const uint32_t info_used)
{
  intersection_info_t new_collision_info[256];
  uint32_t count = 0;
  vector3f *normal = NULL;

  for (uint32_t i = 0, index = 0; i < info_used; ++i) {
    index = collision_info[i].bvh_face_index;
    normal = cvector_as(&bvh->normals, index, vector3f);
    if (dot_product_v3f(velocity, normal) > EPSILON_FLOAT_LOW_PRECISION)
      continue;
    else
      new_collision_info[count++] = collision_info[i];
  }

  memcpy(collision_info, new_collision_info, sizeof(new_collision_info));
  return count;
}

/**
 * Sorts the collision information into buckets where each buckets corresponds 
 * to a number of colinear faces. Then we remove redundant buckets, redundant 
 * buckets are those that have a negating pair (colinear but normals oppose) and
 * could be subject to removal under certain conditions.
 * returns the number of faces left, sorted consecutively.
 */
uint32_t
process_collision_info(
  bvh_t *const bvh, 
  const vector3f *velocity, 
  intersection_info_t collision_info[256], 
  uint32_t info_used)
{
  if (info_used) {
    uint32_t buckets[256];
    uint32_t bucket_count = 
      sort_in_buckets(bvh, collision_info, info_used, buckets);
    info_used = 
      process_buckets(bvh, collision_info, info_used, buckets, bucket_count);
  }

  return trim_backfacing(bvh, velocity, collision_info, info_used);
}