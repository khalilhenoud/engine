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
#include <assert.h>
#include <collision/simple_primitives_collision.h>


// TODO: Change this, this should follow the segments intersection, with a 
// classification and minimal connecting segment, or an out param.
// Do the same for all remaining segments.
collision_result_t
collision_spheres(
  const sphere_t *source, 
  const sphere_t *target)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.direction.data[1] = 1.f;
  result.penetration = -1.f;

  {
    vector3f dx;
    vector3f_set_diff_v3f(&dx, &target->center, &source->center);
    {
      // TODO: consider using a squared version of this to avoid sqrt().
      // TODO: I need to return whether the spheres contain one another.
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
    // TODO: Also check if this produces optimized assembly.
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
    vector3f_set_diff_v3f(&v2, &face->points[0], &face->points[2]);
    normal = cross_product_v3f(&v1, &v2);
    normalize_set_v3f(&normal);

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

        if (
          (dot[0] <= 0.f && dot[1] <= 0.f && dot[2] <= 0.f) ||
          (dot[0] >= 0.f && dot[1] >= 0.f && dot[2] >= 0.f)) {
          int32_t center_on_plane = nextafterf(fabs(distance), 0.f) == 0.f;
          result.direction = center_on_plane ? normal : to_sphere_center;
          normalize_set_v3f(&result.direction);
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
collision_sphere_faces(
  const sphere_t* source, 
  const faces_t* face)
{
  collision_result_t result;
  memset(&result, 0, sizeof(result));
  result.penetration = -1.f;

  // TODO: Implement this.

  return result;
}

collision_result_t
collision_sphere_capsule(
  const sphere_t *source, 
  const capsule_t *capsule)
{
  vector3f direction = { 0.f, 1.f, 0.f };
  vector3f a_capsule, b_capsule, on_capsule; 
  mult_set_v3f(&direction, capsule->half_height);
  vector3f_set_diff_v3f(&a_capsule, &direction, &capsule->center); // a is in the -y.
  mult_set_v3f(&direction, -1.f);
  vector3f_set_diff_v3f(&b_capsule, &direction, &capsule->center); // b is in the +y
  on_capsule = closest_point_on_segment(&source->center, &a_capsule, &b_capsule);

  // TODO: Deal with the case where both sphere share the same origin.
  {
    sphere_t sphere_on_capsule = { on_capsule, capsule->radius };
    return collision_spheres(source, &sphere_on_capsule);
  } 
}

collision_result_t
collision_capsules(
  const capsule_t *source, 
  const capsule_t *target)
{
  vector3f direction_source = { 0.f, 1.f, 0.f };
  vector3f direction_target = direction_source;
  vector3f a_source, b_source, a_target, b_target, on_source, on_target; 
  
  // compute the source capsule endpoints.
  {
    mult_set_v3f(&direction_source, source->half_height);
    vector3f_set_diff_v3f(&a_source, &direction_source, &source->center); // a is in the -y.
    mult_set_v3f(&direction_source, -1.f);
    vector3f_set_diff_v3f(&b_source, &direction_source, &source->center); // b is in the +y
  }

  // compute the target capsule endpoints.
  {
    mult_set_v3f(&direction_target, target->half_height);
    vector3f_set_diff_v3f(&a_target, &direction_target, &target->center); // a is in the -y.
    mult_set_v3f(&direction_target, -1.f);
    vector3f_set_diff_v3f(&b_target, &direction_target, &target->center); // b is in the +y
  }
  
  // find the closest points on the capsules.
  {
    vector3f on_target_a, a_closest, on_target_b, b_closest;
    on_target_a = closest_point_on_segment(&a_source, &a_target, &b_target);
    vector3f_set_diff_v3f(&a_closest, &a_source, &on_target_a);
    on_target_b = closest_point_on_segment(&b_source, &a_target, &b_target);
    vector3f_set_diff_v3f(&b_closest, &b_source, &on_target_b);
    on_target = (length_v3f(&a_closest) < length_v3f(&b_closest)) ? on_target_a : on_target_b;
    on_source = closest_point_on_segment(&on_target, &a_source, &b_source);
  
    // TODO: Deal with the case where both sphere share the same origin.
    {
      sphere_t sphere_source = { on_source, source->radius };
      sphere_t sphere_target = { on_target, target->radius };
      return collision_spheres(&sphere_source, &sphere_target);
    }
  }
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

// NOTE: right now it is for use with classify_segments strictly.
static
primitives_classification_t
find_coplanar_segments_bridge(
  const segment_t *first,
  const segment_t *second,
  segment_t *out)
{
  primitives_classification_t result = CLASSIFIED_DISTINCT;
  assert(out != NULL);

  // TODO: add asserts making sure the lines are coplanar, but not parallel.
  // better still separate the classification from the intersection code, that 
  // way we can generalize the code and use it where we need.
  {
    vector3f 
        ab, ab_normalized, 
        cd, cd_normalized;
    vector3f_set_diff_v3f(&ab, &first->points[0], &first->points[1]);
    ab_normalized = normalize_v3f(&ab);
    vector3f_set_diff_v3f(&cd, &second->points[0], &second->points[1]);
    cd_normalized = normalize_v3f(&cd);

    {
      float xa = first->points[0].data[0];
      float ya = first->points[0].data[1];
      float za = first->points[0].data[2];
      float xc = second->points[0].data[0];
      float yc = second->points[0].data[1];
      float zc = second->points[0].data[2];
      float deltax = ab.data[0];
      float deltay = ab.data[1];
      float deltaz = ab.data[2];
      float deltax_prime = cd.data[0];
      float deltay_prime = cd.data[1];
      float deltaz_prime = cd.data[2];
      float t, t_prime;
      int32_t found = 0;
      float denom = 0.f;

      // [a:1] {{ 30, 0, -100 }, { 35, 0, -90 }}, {{ 0, -50, -80 }, { 5, 25, -100 }}
      // [a:2] {{ 30, 0, -100 }, { 30, 0, -90 }}, {{ 0, -50, -100 }, { 20, -50, -90 }}
      if (nextafterf(deltax_prime, 0.f) != 0.f) {
        if (
          (denom = (deltay_prime / deltax_prime) * deltax - deltay) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = ya - yc + (deltay_prime / deltax_prime) * (xc - xa);
          t /= denom;
        } else if (
          (denom = (deltaz_prime / deltax_prime) * deltax - deltaz) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = za - zc + (deltaz_prime / deltax_prime) * (xc - xa);
          t /= denom;
        }

        if (found) 
          t_prime = (xa - xc + deltax * t) / deltax_prime;
      }

      // [b:1] {{ 30, 50, -100 }, { 35, 0, -90 }}, {{ 0, -50, -100 }, { 0, -70, -90 }}
      // [b:2] {{ 30, 50, -100 }, { 30, 0, -90 }}, {{ 0, -50, -100 }, { 0, -70, -90 }}
      if (!found && nextafterf(deltay_prime, 0.f) != 0.f) {
        if (
          (denom = (deltax_prime / deltay_prime) * deltay - deltax) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = xa - xc + (deltax_prime / deltay_prime) * (yc - ya);
          t /= denom;
        } else if (
          (denom = (deltaz_prime / deltay_prime) * deltay - deltaz) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = za - zc + (deltaz_prime / deltay_prime) * (yc - ya);
          t /= denom;
        }

        if (found)
          t_prime = (ya - yc + deltay * t) / deltay_prime;
      }

      // [c:1] {{ 30, 50, -100 }, { 20, 0, -90 }}, {{ 0, -70, -80 }, { 0, -70, -90 }}
      // [c:2] {{ 30, 50, -100 }, { 30, 0, -90 }}, {{ 0, -70, -80 }, { 0, -70, -90 }}
      if (!found && nextafterf(deltaz_prime, 0.f) != 0.f) {
        if (
          (denom = (deltax_prime / deltaz_prime) * deltaz - deltax) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = xa - xc + (deltax_prime / deltaz_prime) * (zc - za);
          t /= denom;
        } else if (
          (denom = (deltay_prime / deltaz_prime) * deltaz - deltay) && 
          nextafterf(denom, 0.f) != 0.f) {
          found = 1;
          t = ya - yc + (deltay_prime / deltaz_prime) * (zc - za);
          t /= denom;
        }

        if (found)
          t_prime = (za - zc + deltaz * t) / deltaz_prime;
      }

      assert(found);

      // if the intersection occurs outside the segment boundaries.
      if (t < 0.f || t > 1.f || t_prime < 0.f || t_prime > 1.f) {
        vector3f a_proj = closest_point_on_segment(&first->points[0], &second->points[0], &second->points[1]);
        vector3f b_proj = closest_point_on_segment(&first->points[1], &second->points[0], &second->points[1]);
        vector3f a_a_proj = diff_v3f(&first->points[0], &a_proj);
        vector3f b_b_proj = diff_v3f(&first->points[1], &b_proj);
        vector3f c_proj = closest_point_on_segment(&second->points[0], &first->points[0], &first->points[1]);
        vector3f d_proj = closest_point_on_segment(&second->points[1], &first->points[0], &first->points[1]);
        vector3f c_c_proj = diff_v3f(&second->points[0], &c_proj);
        vector3f d_d_proj = diff_v3f(&second->points[1], &d_proj);
        vector3f starting[4] = { first->points[0], first->points[1], second->points[0], second->points[1] };
        vector3f projected[4] = { a_proj, b_proj, c_proj, d_proj };
        vector3f vec[4] = { a_a_proj, b_b_proj, c_c_proj, d_d_proj };
        int32_t index = 0;
        float min = length_v3f(vec + 0);
        float new_min;

        for (int32_t i = 1; i < 4; ++i) {
          new_min = length_v3f(vec + i);
          if (new_min < min) {
            min = new_min;
            index = i;
          }
        }

        // we guarantee the first point to belong to the first segment.
        if (index < 2) {
          out->points[0] = starting[index];
          out->points[1] = projected[index];
        } else {
          out->points[0] = projected[index];
          out->points[1] = starting[index];
        }
        result = CLASSIFIED_COPLANAR_NO_INTERSECT;
      } else {
        t = t < 0.f ? 0.f : t;
        t = t > 1.f ? 1.f : t;
        t_prime = t_prime < 0.f ? 0.f : t_prime;
        t_prime = t_prime > 1.f ? 1.f : t_prime;
        // the points are identical.
        out->points[0].data[0] = xa + deltax * t;
        out->points[0].data[1] = ya + deltay * t;
        out->points[0].data[2] = za + deltaz * t;
        out->points[1].data[0] = xc + deltax_prime * t_prime;
        out->points[1].data[1] = yc + deltay_prime * t_prime;
        out->points[1].data[2] = zc + deltaz_prime * t_prime;
        result = CLASSIFIED_COPLANAR_INTERSECT;
      }
    }
  }

  return result;
}

primitives_classification_t
classify_segments(
  const segment_t *first, 
  const segment_t *second, 
  segment_t *out)
{
  primitives_classification_t result = CLASSIFIED_DISTINCT;
  assert(out != NULL);

  // test if ab, cd are collapsed, assert if true.
  {
    vector3f ab_segment, cd_segment;
    vector3f_set_diff_v3f(&ab_segment, first->points + 0, first->points + 1);
    vector3f_set_diff_v3f(&cd_segment, second->points + 0, second->points + 1);
    assert(nextafterf(length_v3f(&ab_segment), 0.f) != 0.f);
  }

  // test if both segments are identical.
  if ((equal_to_v3f(first->points + 0, second->points + 0) && equal_to_v3f(first->points + 1, second->points + 1)) || 
      (equal_to_v3f(first->points + 0, second->points + 1) && equal_to_v3f(first->points + 1, second->points + 0))) {
      *out= *first;
      return CLASSIFIED_IDENTICAL;
    }

  // calculate the dot product between the segments.
  {
    float dot1, dot2;
    vector3f 
      ab, ab_normalized, 
      cd, cd_normalized;
    vector3f_set_diff_v3f(&ab, &first->points[0], &first->points[1]);
    ab_normalized = normalize_v3f(&ab);
    vector3f_set_diff_v3f(&cd, &second->points[0], &second->points[1]);
    cd_normalized = normalize_v3f(&cd);
    dot1 = dot_product_v3f(&ab_normalized, &cd_normalized);

    // if true then the segments are parallel, we test if they are colinear.
    if (nextafterf(fabs(dot1), 1.f) == 1.f) {
      vector3f from_to;
      vector3f_set_diff_v3f(&from_to, first->points + 0, second->points + 0);

      // if ac are identical we use ad.
      if (nextafterf(length_v3f(&from_to), 0.f) == 0.f)
        vector3f_set_diff_v3f(&from_to, first->points + 0, second->points + 1);

      // both connecting segments cannot be identical.
      assert(nextafterf(length_v3f(&from_to), 0.f) != 0.f);
      normalize_set_v3f(&from_to);
      dot2 = dot_product_v3f(&ab_normalized, &from_to);

      // if true than the segments are colinear, otherwise simply parallel.
      if (nextafterf(fabs(dot2), 1.f) == 1.f) {
        float ac_cb, ad_db, ca_ad, cb_bd, ac_cd, ad_dc;
        vector3f ac, ca, cb, bc, ad, db, bd, dc;
        vector3f_set_diff_v3f(&ac, first->points + 0, second->points + 0);
        ca = mult_v3f(&ac, -1.f);
        vector3f_set_diff_v3f(&cb, second->points + 0, first->points + 1);
        bc = mult_v3f(&cb, -1.f);
        vector3f_set_diff_v3f(&ad, first->points + 0, second->points + 1);
        vector3f_set_diff_v3f(&db, second->points + 1, first->points + 1);
        bd = mult_v3f(&db, -1.f);
        dc = mult_v3f(&cd, -1.f);
        // NOTE: do not normalize, we are only interested in the sign,
        // normalizing would result in division by zero for collapsed vectors.
        ac_cb = dot_product_v3f(&ac, &cb);
        ad_db = dot_product_v3f(&ad, &db);
        ca_ad = dot_product_v3f(&ca, &ad);
        cb_bd = dot_product_v3f(&cb, &bd);
        ac_cd = dot_product_v3f(&ac, &cd);
        ad_dc = dot_product_v3f(&ad, &dc);

        // since we are colinear we check for the overlap.
        if (ac_cb >= 0.f && ad_db >= 0.f) {
          result = CLASSIFIED_COLINEAR_FULL_OVERLAP;
          *out = *second;
        } else if (ca_ad >= 0.f && cb_bd >= 0.f) {
          result = CLASSIFIED_COLINEAR_FULL_OVERLAP;
          *out = *first;
        } else if (ac_cb * ad_db < 0.f) {
          result = CLASSIFIED_COLINEAR_OVERLAPPING;
          if (ac_cb > 0.f) {
            out->points[0] = second->points[0];
            out->points[1] = (ac_cd > 0.f) ? first->points[1] : first->points[0];
          } else {
            out->points[0] = second->points[1];
            out->points[1] = (ad_dc > 0.f) ? first->points[1] : first->points[0];
          }
        } else {
          result = CLASSIFIED_COLINEAR_NO_OVERLAP;
          // we set the out to the closest gap.
          vector3f vec[4] = { ac, ad, bc, bd };
          segment_t seg[4] = { 
            {first->points[0], second->points[0]}, 
            {first->points[0], second->points[1]}, 
            {first->points[1], second->points[0]}, 
            {first->points[1], second->points[1]} };
          float min = length_v3f(&vec[0]);
          float new_min;
          int32_t index = 0;

          for (int32_t i = 1; i < 4; ++i) {
            new_min = length_v3f(&vec[i]);
            if (new_min < min) {
              min = new_min;
              index = i;
            }
          }
          *out = seg[index];
        }
      } else {
        // parallel segments, find minimal connecting segment.
        vector3f a_proj = closest_point_on_segment(&first->points[0], &second->points[0], &second->points[1]);
        vector3f b_proj = closest_point_on_segment(&a_proj, &first->points[0], &first->points[1]);
        out->points[0] = a_proj;
        out->points[1] = b_proj;
        result = CLASSIFIED_PARALLEL;
      }
    } else {
      // the segments are guaranteed not parallel at this point.
      float distance;
      vector3f normal, ac;
      normal = cross_product_v3f(&ab_normalized, &cd_normalized);
      normalize_set_v3f(&normal);
      vector3f_set_diff_v3f(&ac, first->points + 0, second->points + 0);
      distance = dot_product_v3f(&ac, &normal);

      // already coplanar.
      if (distance == 0.f)
        result = find_coplanar_segments_bridge(first, second, out);
      else {
        // make coplanar, and re-translate the result.
        vector3f offset = mult_v3f(&normal, distance);
        segment_t translated = *first;
        add_set_v3f(translated.points + 0, &offset);
        add_set_v3f(translated.points + 1, &offset);
        result = find_coplanar_segments_bridge(&translated, second, out);
        mult_set_v3f(&offset, -1.f);
        add_set_v3f(out->points + 0, &offset);
      }
    }
  }

  return result;
}