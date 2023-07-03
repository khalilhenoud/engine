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
#include <collision/capsule.h>


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
    
    {
      // nudge to plane, basically error correction.
      float distance = get_point_distance(face, normal, intersection);
      vector3f scaled_normal = *normal;
      scaled_normal = mult_v3f(&scaled_normal, -distance);
      add_set_v3f(intersection, &scaled_normal);
    }

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

  {
    // check if the projected sphere center is in the face.
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
        return SPHERE_FACE_COLLIDES_SPHERE_CENTER_IN_FACE;
      } else {
        vector3f_set_diff_v3f(penetration, &projected, &sphere->center);
        normalize_set_v3f(penetration);
        mult_set_v3f(penetration, scale);
        return SPHERE_FACE_COLLIDES;
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
      return SPHERE_FACE_COLLIDES;
    }
  }
}

capsule_face_classification_t
classify_capsule_face(
  const capsule_t* capsule,
  const face_t* face,
  const vector3f* normal,
  vector3f* penetration,
  segment_t* partial_overlap)
{
  segment_t segment;
  get_capsule_segment(capsule, &segment);
  
  assert(penetration != NULL);
  assert(partial_overlap != NULL);

  {
    // get the intersection of the segment with the plane.
    float t;
    point3f intersection;
    segment_plane_classification_t classify_segment = 
      classify_segment_face(face, normal, &segment, &intersection, &t);
    
    if (classify_segment == SEGMENT_PLANE_COPLANAR) {
      coplanar_point_classification_t p_classify[2];
      point3f closest[2];
      p_classify[0] = classify_coplanar_point_face(
        face, normal, segment.points + 0, closest + 0);
      p_classify[1] = classify_coplanar_point_face(
        face, normal, segment.points + 1, closest + 1);

      if (p_classify[0] == p_classify[1]) {

        // both points are inside the face or on the periphery.
        if (p_classify[0] == COPLANAR_POINT_ON_OR_INSIDE) {
          *penetration = *normal;
          mult_set_v3f(penetration, capsule->radius);
          return CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE;
        } else {
          int32_t intersects_face = 0;

          {
            // try and find the partial_overlap in case there is intersection.
            uint32_t jval[] = { 1, 2, 0, (uint32_t)-1 };
            uint32_t vertex_index = 0;

            for (uint32_t i = 0, j = jval[i]; i < 3; ++i, j = jval[i]) {
              segments_classification_t seg_classify;
              segment_t face_segment, out_val;
              face_segment.points[0] = face->points[i];
              face_segment.points[1] = face->points[j];
              seg_classify = classify_segments(
                &face_segment, &segment, &out_val);

              assert(
                seg_classify != SEGMENTS_IDENTICAL &&
                seg_classify != SEGMENTS_COLINEAR_OVERLAPPING &&
                seg_classify != SEGMENTS_COLINEAR_OVERLAPPING_AT_POINT &&
                seg_classify != SEGMENTS_DISTINCT &&
                "the segments cannot be distinct if already coplanar!");

              if (seg_classify == SEGMENTS_COPLANAR_INTERSECT) {
                intersects_face = 1;
                partial_overlap->points[vertex_index++] = out_val.points[0];
              } else if (seg_classify == SEGMENTS_COLINEAR_FULL_OVERLAP) {
                intersects_face = 1;
                assert(
                  vertex_index == 0 && 
                  "there cannot be a prior intersection if there is full overlap!");
                vertex_index = 2;
                partial_overlap->points[0] = face_segment.points[0];
                partial_overlap->points[1] = face_segment.points[1];
                break;
              }
            }

            if (intersects_face == 1) {
              assert(
                vertex_index == 2 &&
                "if there is an intersection we need to have 2 points, as both"
                "segments original points are outside the face!");
              
              *penetration = *normal;
              mult_set_v3f(penetration, capsule->radius);
              return CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE;
            } else
              assert(
                vertex_index == 0 &&
                "if there is no intersection vertex_index should be 0!");
          }

          // both points are outside the face, this devolves into sphere-face
          // classify. sphere-center would be the closest point on the segment.
          if (intersects_face == 0) {
            point3f on_capsule[3];
            uint32_t index = 0;
            on_capsule[0] = closest_point_on_segment(
              face->points + 0, &segment);
            on_capsule[1] = closest_point_on_segment(
              face->points + 1, &segment);
            on_capsule[2] = closest_point_on_segment(
              face->points + 2, &segment);

            {
              vector3f diff = diff_v3f(face->points + 0, on_capsule + 0);
              float min_dis = length_squared_v3f(&diff), temp = 0.f;
              for (uint32_t i = 1; i < 3; ++i) {
                diff = diff_v3f(face->points + i, on_capsule + i);
                temp = length_squared_v3f(&diff);
                if (temp < min_dis) {
                  min_dis = temp;
                  index = i;
                }
              }
            }

            {
              sphere_t sphere = { on_capsule[index], capsule->radius };
              sphere_face_classification_t classify_sphere = 
                classify_sphere_face(&sphere, face, normal, penetration);

              return classify_sphere == SPHERE_FACE_NO_COLLISION ?
                CAPSULE_FACE_NO_COLLISION : 
                CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE;
            }
          }
        }
      } else {
        // the segment has one point inside the face and one outside, we need to
        // find the partial overlap. the segment will intersect one side or 2 of
        // the face at max. This can be the case if the intersection point is at
        // a corner.
        int32_t found = 0;
        uint32_t index_on, index_out;
        index_on = p_classify[0] == COPLANAR_POINT_ON_OR_INSIDE ? 0 : 1;
        index_out = (index_on + 1) % 2;

        partial_overlap->points[0] = 
        partial_overlap->points[1] = segment.points[index_on];

        // if any of the face points is on the segment, that would make it the
        // second point of the overlap and that would be the end of that.
        // otherwise, we need to check intersection of the segment with each
        // of the face segments.
        for (uint32_t i = 0; i < 3; ++i) {
          point3f candidate = closest_point_on_segment(
            face->points + i, 
            &segment);
          vector3f diff = diff_v3f(&candidate, face->points + i);
          if (nextafterf(length_squared_v3f(&diff), 0.f) == 0.f) {
            // the point is on the segment.
            diff = diff_v3f(
              partial_overlap->points + 1,
              &candidate);
            // if it isn't already set as the second point.
            if (nextafterf(length_v3f(&diff), 0.f) != 0.f) {
              partial_overlap->points[1] = face->points[i];
              found = 1;
              break;
            }
          }
        }

        // NOTE: Due to imprecision, there is no guarantee that segment
        // classify will return what we expect so I should use the data I
        // already have. I need to know if the in point belongs to any of the
        // segments of the face.
        if (!found) {
          uint32_t jval[] = { 1, 2, 0, (uint32_t)-1 };
          for (uint32_t i = 0, j = jval[i]; i < 3; ++i, j = jval[i]) {
            segments_classification_t seg_classify;
            segment_t face_segment, out_val;
            // NOTE: the normal to the face segment is segment x face normal
            face_segment.points[0] = face->points[i];
            face_segment.points[1] = face->points[j];
            seg_classify = classify_segments(
              &face_segment, &segment, &out_val);
            
            assert(
              seg_classify != SEGMENTS_COLINEAR_OVERLAPPING &&
              "this should have been cought in the point test preceeding this!"
              );

            if (seg_classify == SEGMENTS_COPLANAR_INTERSECT) {
              found = 1;
              vector3f diff = diff_v3f(
                partial_overlap->points + 1, 
                out_val.points);
              if (nextafterf(length_v3f(&diff), 0.f) != 0.f) {
                partial_overlap->points[1] = out_val.points[0];
                break;
              }
            }
          }

          assert(
            found == 1 && 
            "at this point both overlap points should have been found even if" 
            "they are the same!!");
        }

        *penetration = *normal;
        mult_set_v3f(penetration, capsule->radius);
        return CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE;
      }
    } else if (classify_segment == SEGMENT_PLANE_PARALLEL) {
      vector3f vec, p[3];
      float dist[3];
      point3f on_capsule_axis;
      p[0] = closest_point_on_segment(face->points + 0, &segment);
      p[1] = closest_point_on_segment(face->points + 1, &segment);
      p[2] = closest_point_on_segment(face->points + 2, &segment);
      vector3f_set_diff_v3f(&vec, p + 0, face->points + 0);
      dist[0] = length_v3f(&vec);
      vector3f_set_diff_v3f(&vec, p + 1, face->points + 1);
      dist[1] = length_v3f(&vec);
      vector3f_set_diff_v3f(&vec, p + 2, face->points + 2);
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

        on_capsule_axis = p[index];
      }

      {
        sphere_t sphere = { on_capsule_axis, capsule->radius };
        sphere_face_classification_t classify_sphere = 
          classify_sphere_face(&sphere, face, normal, penetration);

        assert(
          classify_sphere != SPHERE_FACE_COLLIDES_SPHERE_CENTER_IN_FACE &&
          "We already know the capsule axis is parallel but not coplanar!!");

        return classify_sphere == SPHERE_FACE_NO_COLLISION ?
          CAPSULE_FACE_NO_COLLISION : CAPSULE_FACE_COLLIDES;
      }
    } else {
      // the segment does intersect the face plane.
      point3f closest;
      coplanar_point_classification_t classify_point = 
        classify_coplanar_point_face(face, normal, &intersection, &closest);

      if (
        classify_segment == SEGMENT_PLANE_INTERSECT_OFF_SEGMENT ||
        classify_point == COPLANAR_POINT_OUTSIDE) {
        // either the intersection point is outside the face or it is inside but
        // outside the boundaries of the segment.
        point3f on_capsule_axis = closest_point_on_segment(&closest, &segment);
        sphere_t sphere = { on_capsule_axis, capsule->radius };
        sphere_face_classification_t classify_sphere = 
          classify_sphere_face(&sphere, face, normal, penetration);

        return classify_sphere == SPHERE_FACE_NO_COLLISION ?
          CAPSULE_FACE_NO_COLLISION : CAPSULE_FACE_COLLIDES;
      } else {
        // the intersection point is within the face and wihin the segment
        // boundaries. the penetration in this case indicates the distance we
        // translate along the normal to avoid a collision with the face.
        float distance[] = { 0, 0, 0};
        distance[0] = get_point_distance(face, normal, segment.points + 0);
        distance[1] = get_point_distance(face, normal, segment.points + 1);
        assert((distance[0] <= 0.f || distance[1] <= 0.f) && 
          "One of the distances has to be negative!");
        distance[2] = distance[0] <= 0.f ? -distance[0] : distance[1];
        distance[2] += capsule->radius;
        *penetration = *normal;
        mult_set_v3f(penetration, distance[2]);
        return CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_INTERSECTS_FACE;
      }
    }
  }

  return CAPSULE_FACE_NO_COLLISION;
}