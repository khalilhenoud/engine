/**
 * @file basic_shapes_collision.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-04-27
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <string.h>
#include <math.h>
#include <collision/simple_primitives_collision.h>


collision_result_t
collision_spheres(
  const sphere_t *source, 
  const sphere_t *target)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  {
    vector3f dx;
    vector3f_set_diff_v3f(&dx, &target->center, &source->center);
    {
      // TODO: consider using a squared version of this to avoid sqrt().
      float distance = length_v3f(&dx);
      float radius_comb = source->radius + target->radius;
      if (distance < radius_comb) {
        result.penetration = radius_comb - distance;
        result.direction = normalize_v3f(&dx);
      }
    }
  }

  return result;
}

// TODO: move this to utils.
static 
vector3f 
closest_point_on_segment(
  const vector3f* point, 
  const vector3f* a, 
  const vector3f* b)
{
  float proj_length, ab_length;
  vector3f a_point, a_b, a_b_normalized, result;
  vector3f_set_diff_v3f(&a_point, a, point);
  vector3f_set_diff_v3f(&a_b, a, b);
  a_b_normalized = normalize_v3f(&a_b);
  proj_length = dot_product_v3f(&a_point, &a_b_normalized) / length_v3f(&a_b);
  proj_length = proj_length < 0.f ? 0.f : proj_length;
  proj_length = proj_length > 1.f ? 1.f : proj_length;
  {
    vector3f ab_scaled = mult_v3f(&a_b, proj_length);
    // TODO: These functions needs a variant that does not take a pointer.
    // TODO: Also check if this produces optimzied assembly.
    result = add_v3f(a, &ab_scaled);
  }
  return result;
}

collision_result_t
collision_sphere_face(
  const sphere_t *source, 
  const face_t *face)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  {
    vector3f normal, v1, v2;
    vector3f_set_diff_v3f(&v1, &face->points[0], &face->points[1]);
    normalize_set_v3f(&v1);
    vector3f_set_diff_v3f(&v2, &face->points[0], &face->points[2]);
    normalize_set_v3f(&v2);
    normal = cross_product_v3f(&v1, &v2);

    // check the distance of the center of the sphere to the plane.
    {
      float distance;
      vector3f to_sphere;
      vector3f_set_diff_v3f(&to_sphere, &face->points[0], &source->center);
      distance = dot_product_v3f(&normal, &to_sphere);
      
      // the sphere distance to the infinite plane is greater than its radius.
      if (fabs(distance) > source->radius)
        return result;

      // check if the projected sphere center is in the face.
      {
        vector3f to_sphere_center;
        vector3f projected;
        vector3f v01, v12, v20;
        vector3f v0c, v1c, v2c;
        vector3f c[3];
        float dot[3] = { 0 };
        
        to_sphere_center = mult_v3f(&normal, distance);

        vector3f_set_diff_v3f(&projected, &to_sphere_center, &source->center);
        vector3f_set_diff_v3f(&v01, &face->points[0], &face->points[1]);
        vector3f_set_diff_v3f(&v0c, &face->points[0], &projected);
        c[0] = cross_product_v3f(&v01, &v0c);
        dot[0] = dot_product_v3f(c + 0, &normal);
        
        vector3f_set_diff_v3f(&v12, &face->points[1], &face->points[2]);
        vector3f_set_diff_v3f(&v1c, &face->points[1], &projected);
        c[1] = cross_product_v3f(&v12, &v1c);
        dot[1] = dot_product_v3f(c + 1, &normal);
        
        vector3f_set_diff_v3f(&v20, &face->points[2], &face->points[0]);
        vector3f_set_diff_v3f(&v2c, &face->points[2], &projected);
        c[2] = cross_product_v3f(&v20, &v2c);
        dot[2] = dot_product_v3f(c + 2, &normal);

        if ((dot[0] <= 0.f && dot[1] <= 0.f && dot[2] <= 0.f) ||
            (dot[0] >= 0.f && dot[1] >= 0.f && dot[2] >= 0.f)) {
            result.direction = to_sphere_center;
            result.penetration = source->radius - fabs(distance);
            return result;
          }
      }

      // projected center of the sphere is not within the triangle.
      {
        vector3f vec, p[3];
        float dist[3];
        p[0] = closest_point_on_segment(
          &source->center, 
          &face->points[0], 
          &face->points[1]);
        p[1] = closest_point_on_segment(
          &source->center, 
          &face->points[1], 
          &face->points[2]);
        p[2] = closest_point_on_segment(
          &source->center, 
          &face->points[2], 
          &face->points[0]);
        vector3f_set_diff_v3f(&vec, p + 0, &source->center);
        dist[0] = length_v3f(&vec);
        vector3f_set_diff_v3f(&vec, p + 1, &source->center);
        dist[1] = length_v3f(&vec);
        vector3f_set_diff_v3f(&vec, p + 2, &source->center);
        dist[2] = length_v3f(&vec);

        if (
          dist[0] < source->radius || 
          dist[1] < source->radius || 
          dist[2] < source->radius) {
            float minimum = source->radius;
            int32_t index = -1;

            for (int32_t i = 0; i < 3; ++i) {
              if (dist[i] < minimum) {
                minimum = dist[i];
                index = i;
              }
            }

            vector3f_set_diff_v3f(&result.direction, p + index, &source->center);
            normalize_set_v3f(&result.direction);
            result.penetration = source->radius - minimum;
            return result;
        }
      }
    }
  }

  return result;
}

collision_result_t
collision_sphere_capsule(
  const sphere_t *source, 
  const capsule_t *capsule)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  return result;
}

collision_result_t
collision_capsules(
  const capsule_t *source, 
  const capsule_t *target)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  return result;
}

collision_result_t
collision_capsule_face(
  const capsule_t *source, 
  const face_t *face)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  return result;
}