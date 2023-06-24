/**
 * @file face.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <math.h>
#include <assert.h>
#include <collision/face.h>
#include <collision/segment.h>
#include <collision/sphere.h>


void
get_faces_normals(
  const face_t *faces, 
  const uint32_t count, 
  vector3f *normals)
{
  vector3f v1, v2;

  assert(normals != NULL);

  for (uint32_t i = 0; i < count; ++i) {
    vector3f_set_diff_v3f(&v1, &faces[i].points[0], &faces[i].points[1]);
    vector3f_set_diff_v3f(&v2, &faces[i].points[0], &faces[i].points[2]);
    normals[i] = cross_product_v3f(&v1, &v2);
    normalize_set_v3f(normals + i);
  }
}

float
get_point_distance(
  const face_t *face, 
  const vector3f *normal, 
  const point3f *point)
{
  vector3f to_point = diff_v3f(&face->points[0], point);
  return dot_product_v3f(normal, &to_point);
}

point3f
get_point_projection(
  const face_t *face, 
  const vector3f *normal, 
  const point3f *point,
  float *distance)
{
  assert(distance != NULL);
  *distance = get_point_distance(face, normal, point);
  
  {
    vector3f scaled_normal = mult_v3f(normal, *distance);
    point3f projected = diff_v3f(&scaled_normal, point);
    return projected;
  }
}

segment_plane_classification_t
classify_segment_face(
  const face_t *face, 
  const vector3f *normal, 
  const segment_t *segment,
  point3f *intersection,
  float *t)
{
  assert(intersection != NULL);
  assert(t != NULL);

  vector3f delta = diff_v3f(segment->points + 0, segment->points + 1);
  const point3f *A = segment->points + 0;

  {
    float dotproduct = dot_product_v3f(&delta, normal);
    if (nextafterf(dotproduct, 0.f) == 0.f) {
      // colinear or parallel.
      float distance = get_point_distance(face, normal, A);
      return (nextafterf(distance, 0.f) == 0.f) ? 
        SEGMENT_PLANE_COPLANAR : 
        SEGMENT_PLANE_PARALLEL;
    }
  }

  {
    float D = 
      - normal->data[0] * face->points[0].data[0] 
      - normal->data[1] * face->points[0].data[1] 
      - normal->data[2] * face->points[0].data[2];
    
    float denom = 
      normal->data[0] * delta.data[0] + 
      normal->data[1] * delta.data[1] + 
      normal->data[2] * delta.data[2];
    assert(nextafterf(denom, 0.f) != 0.f);
    *t = (
      - normal->data[0] * A->data[0] 
      - normal->data[1] * A->data[1] 
      - normal->data[2] * A->data[2] - D) / denom;

    // scale the delta by t, and add it to A to get the intersection.
    mult_set_v3f(&delta, *t);
    *intersection = add_v3f(A, &delta);
    return (*t <= 1.f && *t >= 0.f) ? 
      SEGMENT_PLANE_INTERSECT_ON_SEGMENT : 
      SEGMENT_PLANE_INTERSECT_OFF_SEGMENT;
  }

  return SEGMENT_PLANE_PARALLEL;
}

coplanar_point_classification_t
classify_coplanar_point_face(
  const face_t *face, 
  const vector3f *normal,
  const point3f *point,
  point3f *closest)
{
  assert(closest != NULL);

  {
    // has to be coplanar.
    float distance = get_point_distance(face, normal, point);
    assert(nextafterf(distance, 0.f) == 0.f);
  }

  {
    vector3f v01, v12, v20;
    vector3f v0c, v1c, v2c;
    vector3f c[3];
    float dot[3] = { 0 };

    vector3f_set_diff_v3f(&v01, &face->points[0], &face->points[1]);
    vector3f_set_diff_v3f(&v0c, &face->points[0], point);
    c[0] = cross_product_v3f(&v01, &v0c);
    dot[0] = dot_product_v3f(c + 0, normal);
    
    vector3f_set_diff_v3f(&v12, &face->points[1], &face->points[2]);
    vector3f_set_diff_v3f(&v1c, &face->points[1], point);
    c[1] = cross_product_v3f(&v12, &v1c);
    dot[1] = dot_product_v3f(c + 1, normal);
    
    vector3f_set_diff_v3f(&v20, &face->points[2], &face->points[0]);
    vector3f_set_diff_v3f(&v2c, &face->points[2], point);
    c[2] = cross_product_v3f(&v20, &v2c);
    dot[2] = dot_product_v3f(c + 2, normal);

    if (
      (dot[0] <= 0.f && dot[1] <= 0.f && dot[2] <= 0.f) ||
      (dot[0] >= 0.f && dot[1] >= 0.f && dot[2] >= 0.f)) {
      *closest = *point;
      return COPLANAR_POINT_ON_OR_INSIDE;
    } else {
      vector3f vec, p[3];
      float dist[3];
      p[0] = closest_point_on_segment_loose(
        point, 
        &face->points[0], 
        &face->points[1]);
      p[1] = closest_point_on_segment_loose(
        point, 
        &face->points[1], 
        &face->points[2]);
      p[2] = closest_point_on_segment_loose(
        point, 
        &face->points[2], 
        &face->points[0]);
      vector3f_set_diff_v3f(&vec, p + 0, point);
      dist[0] = length_v3f(&vec);
      vector3f_set_diff_v3f(&vec, p + 1, point);
      dist[1] = length_v3f(&vec);
      vector3f_set_diff_v3f(&vec, p + 2, point);
      dist[2] = length_v3f(&vec);

      {
        float minimum = dist[0];
        uint32_t index = 0;

        for (uint32_t i = 1; i < 3; ++i) {
          if (dist[i] < minimum) {
            minimum = dist[i];
            index = i;
          }
        }

        *closest = *(p + index);
      }

      return COPLANAR_POINT_OUTSIDE;
    }
  }
}

point_halfspace_classification_t
classify_point_halfspace(
  const face_t *face, 
  const vector3f *normal, 
  const point3f *point)
{
  float distance = get_point_distance(face, normal, point);
  if (nextafterf(distance, 0.f) == 0.f) 
    return POINT_ON_PLANE;
  else if (distance > 0.f)
    return POINT_IN_POSITIVE_HALFSPACE;
  else 
    return POINT_IN_NEGATIVE_HALFSPACE;
}

sphere_face_classification_t
classify_sphere_face(
  const sphere_t *sphere, 
  const face_t *face, 
  const vector3f *normal,
  vector3f* penetration)
{
  float distance = 0.f;
  vector3f projected = get_point_projection(
    face, 
    normal, 
    &sphere->center, 
    &distance);
  
  // the sphere distance to the infinite plane is greater than its radius.
  if (fabs(distance) > sphere->radius)
    return SPHERE_FACE_NO_COLLISION;

  // check if the projected sphere center is in the face.
  {
    vector3f closest_on_face;
    coplanar_point_classification_t classification;
    classification = classify_coplanar_point_face(
      face, 
      normal, 
      &projected, 
      &closest_on_face);

    if (classification == COPLANAR_POINT_ON_OR_INSIDE) {
      float scale = sphere->radius - fabs(distance);
      if (nextafterf(fabs(distance), 0.f) == 0.f) {
        *penetration = *normal;
        mult_set_v3f(penetration, scale);
        return SPHERE_FACE_COLLIDES_CENTER_COPLANAR_TO_FACE;
      } else {
        vector3f_set_diff_v3f(penetration, &projected, &sphere->center);
        normalize_set_v3f(penetration);
        mult_set_v3f(penetration, scale);
        return distance > 0.f ? 
          SPHERE_FACE_COLLIDES_CENTER_IN_POSITIVE_SPACE : 
          SPHERE_FACE_COLLIDES_CENTER_IN_NEGATIVE_SPACE;
      }
    } else {
      vector3f from_to = diff_v3f(&closest_on_face, &sphere->center);
      float from_to_length = length_v3f(&from_to);
      float scale = sphere->radius - from_to_length;

      if (from_to_length > sphere->radius)
        return SPHERE_FACE_NO_COLLISION;

      normalize_set_v3f(&from_to);
      mult_set_v3f(&from_to, scale);
      *penetration = from_to;
      if (nextafterf(fabs(distance), 0.f) == 0.f)
        return SPHERE_FACE_COLLIDES_CENTER_COPLANAR_TO_FACE;
      else
        return distance > 0.f ? 
          SPHERE_FACE_COLLIDES_CENTER_IN_POSITIVE_SPACE : 
          SPHERE_FACE_COLLIDES_CENTER_IN_NEGATIVE_SPACE;
    }
  }
}