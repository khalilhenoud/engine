/**
 * @file capsule.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <math.h>
#include <assert.h>
#include <collision/sphere.h>
#include <collision/segment.h>
#include <collision/capsule.h>


sphere_capsule_classification_t
classify_sphere_capsule(
  const sphere_t *source, 
  const capsule_t *capsule,
  vector3f* penetration)
{
  vector3f a_capsule, b_capsule, on_capsule; 
  
  assert(penetration != NULL);

  get_capsule_segment_loose(capsule, &a_capsule, &b_capsule);
  on_capsule = closest_point_on_segment_loose(
    &source->center, 
    &a_capsule, 
    &b_capsule);

  {
    sphere_t sphere_on_capsule = { on_capsule, capsule->radius };
    spheres_classification_t classification = classify_spheres(
      source,
      &sphere_on_capsule, 
      penetration);

    switch (classification) {
      case SPHERES_DISTINCT:
        return SPHERE_CAPSULE_DISTINCT;
      break;
      case SPHERES_COLLIDE:
        return SPHERE_CAPSULE_INTERSECTS;
      break;
      // both spheres will share the same center, it will be up to the user code
      // to handle these cases.
      case SPHERES_SHARE_CENTER:
      case SPHERES_IDENTICAL:
        return SPHERE_CENTER_WITHIN_CAPSULE_SEGMENT;
      break;
      default:
      assert(0);
      break;
    }
  } 

  return SPHERES_DISTINCT;
}

int32_t
classify_capsules_segments(
  const capsule_t *source, 
  const capsule_t *target, 
  segment_t *result)
{
  vector3f a_source, b_source, a_target, b_target; 

  assert(result != NULL);
  
  get_capsule_segment_loose(source, &a_source, &b_source);
  get_capsule_segment_loose(target, &a_target, &b_target);
  
  {
    // find the closest points on the capsules.
    segment_t segments[3] = {{ a_source, b_source }, { a_target, b_target }};
    segments_classification_t classification = classify_segments(
      segments + 0, 
      segments + 1, 
      segments + 2);
    *result = segments[2];
    return classification;
  }
}

capsules_classification_t
classify_capsules(
  const capsule_t *source, 
  const capsule_t *target, 
  vector3f* penetration)
{
  segment_t bridge;
  segments_classification_t 
  segments_classification = classify_capsules_segments(source, target, &bridge);

  // special cases: it is up to the user code to handle these cases. call 
  // classify_capsules_segments to get the overlap.
  switch (segments_classification) {
  case SEGMENTS_IDENTICAL:
    return CAPSULES_AXIS_IDENTICAL;
    break;
  case SEGMENTS_COLINEAR_OVERLAPPING:
    return CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP;
    break;
  case SEGMENTS_COLINEAR_OVERLAPPING_AT_POINT:
    return CAPSULES_AXIS_COLINEAR_PARTIAL_OVERLAP_AT_POINT;
    break;
  case SEGMENTS_COLINEAR_FULL_OVERLAP:
    return CAPSULES_AXIS_COLINEAR_FULL_OVERLAP;
    break;
  }

  {
    sphere_t sphere_source = { bridge.points[0], source->radius };
    sphere_t sphere_target = { bridge.points[1], target->radius };
    spheres_classification_t spheres_classification = classify_spheres(
      &sphere_source, 
      &sphere_target,
      penetration);

    switch (spheres_classification) {
      case SPHERES_DISTINCT:
        return CAPSULES_DISTINCT;
      break;
      case SPHERES_COLLIDE:
        return CAPSULES_COLLIDE;
      break;
      // both spheres will share the same center, it will be up to the user code
      // to handle these cases.
      case SPHERES_SHARE_CENTER:
      case SPHERES_IDENTICAL:
        return CAPSULES_AXIS_COPLANAR_AND_INTERSECTS;
      break;
      default:
      assert(0);
      break;
    }
  }

  return CAPSULES_DISTINCT;
}

void
get_capsule_segment(
  const capsule_t *source,
  segment_t *segment)
{
  assert(segment != NULL);

  {
    vector3f direction_source = { 0.f, 1.f, 0.f };

    mult_set_v3f(&direction_source, source->half_height);
    vector3f_set_diff_v3f(segment->points + 0, &direction_source, &source->center); // a is in the -y.
    mult_set_v3f(&direction_source, -1.f);
    vector3f_set_diff_v3f(segment->points + 1, &direction_source, &source->center); // b is in the +y
  }
}

void
get_capsule_segment_loose(
  const capsule_t *source,
  point3f *a,
  point3f *b)
{
  assert(a != NULL);
  assert(b != NULL);

   {
    vector3f direction_source = { 0.f, 1.f, 0.f };

    mult_set_v3f(&direction_source, source->half_height);
    vector3f_set_diff_v3f(a, &direction_source, &source->center); // a is in the -y.
    mult_set_v3f(&direction_source, -1.f);
    vector3f_set_diff_v3f(b, &direction_source, &source->center); // b is in the +y
  }
}