/**
 * @file segment.c
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
#include <collision/segment.h>


float
get_point_distance_to_line(
  const point3f* point,
  const line_t* target)
{
  float ab_length, a_point_length, dot, sin_radian;
  vector3f a_point, a_b, a_point_normalized, a_b_normalized;
  vector3f_set_diff_v3f(&a_point, target->points + 0, point);
  vector3f_set_diff_v3f(&a_b, target->points + 0, target->points + 1);
  ab_length = length_v3f(&a_b);
  assert(
    nextafterf(ab_length, 0.f) != 0.f &&
    "We do not support collapsed segments!");

  a_point_length = length_v3f(&a_point);
  if (nextafterf(a_point_length, 0.f) == 0.f)
    return 0.f;
  
  a_b_normalized = div_v3f(&a_b, ab_length);
  a_point_normalized = div_v3f(&a_point, a_point_length);
  dot = dot_product_v3f(&a_b_normalized, &a_point_normalized);
  sin_radian = sinf(acosf(dot));
  return sin_radian * a_point_length;
}

point3f 
closest_point_on_segment(
  const point3f* point, 
  const segment_t* target)
{
  float proj_length, ab_length;
  vector3f a_point, a_b, a_b_normalized, result;
  vector3f_set_diff_v3f(&a_point, target->points + 0, point);
  vector3f_set_diff_v3f(&a_b, target->points + 0, target->points + 1);
  ab_length = length_v3f(&a_b);
  assert(
    nextafterf(ab_length, 0.f) != 0.f &&
    "We do not support collapsed segments!");
  a_b_normalized = normalize_v3f(&a_b);
  proj_length = dot_product_v3f(&a_point, &a_b_normalized) / ab_length;
  proj_length = proj_length < 0.f ? 0.f : proj_length;
  proj_length = proj_length > 1.f ? 1.f : proj_length;

  {
    vector3f ab_scaled = mult_v3f(&a_b, proj_length);
    // TODO: These functions needs a variant that does not take a pointer.
    result = add_v3f(target->points + 0, &ab_scaled);
  }
  return result;
}

point3f 
closest_point_on_segment_loose(
  const point3f* point, 
  const point3f* a,
  const point3f* b)
{
  segment_t target;
  target.points[0] = *a;
  target.points[1] = *b;
  return closest_point_on_segment(point, &target);
}

// NOTE: for internal use only. If we want to generalize then we need to add
// asserts to ensure the segments are coplanar (and not parallel).
static
segments_classification_t
find_coplanar_segments_bridge(
  const segment_t *first,
  const segment_t *second,
  segment_t *out)
{
  segments_classification_t result = SEGMENTS_DISTINCT;
  assert(out != NULL);

  {
    vector3f 
        ab, ab_normalized, 
        cd, cd_normalized;
    vector3f_set_diff_v3f(&ab, &first->points[0], &first->points[1]);
    ab_normalized = normalize_v3f(&ab);
    vector3f_set_diff_v3f(&cd, &second->points[0], &second->points[1]);
    cd_normalized = normalize_v3f(&cd);

    {
      // NOTE: an alternative way to solve this would be a system of linear
      // equations.
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

      // unit test data (first set for the 'if', second for the 'else if'):
      // {{ 30, 0, -100 }, { 35, 0, -90 }}, {{ 0, -50, -80 }, { 5, 25, -100 }}
      // {{ 30, 0, -100 }, { 30, 0, -90 }}, {{ 0, -50, -100 }, { 20, -50, -90 }}
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

      // unit test data (first set for the 'if', second for the 'else if'):
      // {{ 30, 50, -100 }, { 35, 0, -90 }}, {{ 0, -50, -100 }, { 0, -70, -90 }}
      // {{ 30, 50, -100 }, { 30, 0, -90 }}, {{ 0, -50, -100 }, { 0, -70, -90 }}
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

      // unit test data (first set for the 'if', second for the 'else if'):
      // {{ 30, 50, -100 }, { 20, 0, -90 }}, {{ 0, -70, -80 }, { 0, -70, -90 }}
      // {{ 30, 50, -100 }, { 30, 0, -90 }}, {{ 0, -70, -80 }, { 0, -70, -90 }}
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

      // if the intersection is outside the segment boundaries.
      if (t < 0.f || t > 1.f || t_prime < 0.f || t_prime > 1.f) {
        vector3f a_proj = closest_point_on_segment(&first->points[0], second);
        vector3f b_proj = closest_point_on_segment(&first->points[1], second);
        vector3f a_a_proj = diff_v3f(&first->points[0], &a_proj);
        vector3f b_b_proj = diff_v3f(&first->points[1], &b_proj);
        vector3f c_proj = closest_point_on_segment(&second->points[0], first);
        vector3f d_proj = closest_point_on_segment(&second->points[1], first);
        vector3f c_c_proj = diff_v3f(&second->points[0], &c_proj);
        vector3f d_d_proj = diff_v3f(&second->points[1], &d_proj);
        vector3f starting[4] = { 
          first->points[0], first->points[1], 
          second->points[0], second->points[1] };
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

        // we guarantee the first point belong to the 'first' segment.
        if (index < 2) {
          out->points[0] = starting[index];
          out->points[1] = projected[index];
        } else {
          out->points[0] = projected[index];
          out->points[1] = starting[index];
        }
        result = SEGMENTS_COPLANAR_NO_INTERSECT;
      } else {
        t = t < 0.f ? 0.f : t;
        t = t > 1.f ? 1.f : t;
        t_prime = t_prime < 0.f ? 0.f : t_prime;
        t_prime = t_prime > 1.f ? 1.f : t_prime;
        // TODO: these points are identical add asserts to ensure this.
        out->points[0].data[0] = xa + deltax * t;
        out->points[0].data[1] = ya + deltay * t;
        out->points[0].data[2] = za + deltaz * t;
        out->points[1].data[0] = xc + deltax_prime * t_prime;
        out->points[1].data[1] = yc + deltay_prime * t_prime;
        out->points[1].data[2] = zc + deltaz_prime * t_prime;
        result = SEGMENTS_COPLANAR_INTERSECT;
      }
    }
  }

  return result;
}

segments_classification_t
classify_segments(
  const segment_t *first, 
  const segment_t *second, 
  segment_t *out)
{
  segments_classification_t result = SEGMENTS_DISTINCT;
  assert(out != NULL);

  {
    // test if ab, cd are collapsed, assert if that is the case.
    vector3f ab_segment, cd_segment;
    vector3f_set_diff_v3f(&ab_segment, first->points + 0, first->points + 1);
    vector3f_set_diff_v3f(&cd_segment, second->points + 0, second->points + 1);
    assert(nextafterf(length_v3f(&ab_segment), 0.f) != 0.f);
  }

  // test if both segments are identical.
  if ((equal_to_v3f(first->points + 0, second->points + 0) && equal_to_v3f(first->points + 1, second->points + 1)) || 
      (equal_to_v3f(first->points + 0, second->points + 1) && equal_to_v3f(first->points + 1, second->points + 0))) {
      *out = *first;
      return SEGMENTS_IDENTICAL;
    }

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
        // IMPORTANT: do not normalize, we are only interested in the sign,
        // normalizing would result in division by zero for collapsed vectors.
        ac_cb = dot_product_v3f(&ac, &cb);
        ad_db = dot_product_v3f(&ad, &db);
        ca_ad = dot_product_v3f(&ca, &ad);
        cb_bd = dot_product_v3f(&cb, &bd);
        ac_cd = dot_product_v3f(&ac, &cd);
        ad_dc = dot_product_v3f(&ad, &dc);

        // since we are colinear we check for the overlap.
        if (ac_cb >= 0.f && ad_db >= 0.f) {
          result = SEGMENTS_COLINEAR_FULL_OVERLAP;
          *out = *second;
        } else if (ca_ad >= 0.f && cb_bd >= 0.f) {
          result = SEGMENTS_COLINEAR_FULL_OVERLAP;
          *out = *first;
        } else if (ac_cb * ad_db < 0.f) {
          result = SEGMENTS_COLINEAR_OVERLAPPING;
          if (ac_cb > 0.f) {
            out->points[0] = (ac_cd > 0.f) ? first->points[1] : first->points[0];
            out->points[1] = second->points[0];
          } else {
            out->points[0] = (ad_dc > 0.f) ? first->points[1] : first->points[0];
            out->points[1] = second->points[1];
          }
        } else {
          result = SEGMENTS_COLINEAR_OVERLAPPING_AT_POINT;
          if (
            equal_to_v3f(first->points + 0, second->points + 0) ||
            equal_to_v3f(first->points + 0, second->points + 1)) {
            out->points[0] = out->points[1] = first->points[0];
          } else if (
            equal_to_v3f(first->points + 1, second->points + 0) ||
            equal_to_v3f(first->points + 1, second->points + 1)) {
            out->points[0] = out->points[1] = first->points[1];
          } else {
            result = SEGMENTS_COLINEAR_NO_OVERLAP;
            // we set 'out' to the closest gap.
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
        }
      } else {
        // parallel segments, find minimal connecting segment.
        vector3f a_proj = closest_point_on_segment(&first->points[0], second);
        vector3f b_proj = closest_point_on_segment(&a_proj, first);
        out->points[0] = b_proj;
        out->points[1] = a_proj;
        result = SEGMENTS_PARALLEL;
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
      if (nextafterf(distance, 0.f) == 0.f)
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