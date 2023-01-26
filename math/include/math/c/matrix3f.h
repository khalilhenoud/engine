/**
 * @file matrix3f.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef C_MATRIX_3F_H
#define C_MATRIX_3F_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <math.h>
#include <math/c/vector3f.h>
#include <math/c/common.h>


typedef 
enum {
  M3_RC_00,
  M3_RC_01,
  M3_RC_02,
  M3_RC_10,
  M3_RC_11,
  M3_RC_12,
  M3_RC_20,
  M3_RC_21,
  M3_RC_22
} M3_RC_XX;

typedef 
struct {
  float data[9];
} matrix3f;


////////////////////////////////////////////////////////////////////////////////
inline
void
matrix3f_set_identity(matrix3f* dst)
{
  dst->data[M3_RC_00] = dst->data[M3_RC_01] = dst->data[M3_RC_02] = 
  dst->data[M3_RC_10] = dst->data[M3_RC_11] = dst->data[M3_RC_12] = 
  dst->data[M3_RC_20] = dst->data[M3_RC_21] = dst->data[M3_RC_22] = 0.f;
  dst->data[M3_RC_00] = dst->data[M3_RC_11] = dst->data[M3_RC_22] = 1.f;
}

inline
void
matrix3f_copy(matrix3f* dst, const matrix3f* src)
{
  memcpy(dst->data, src->data, sizeof(dst->data));
}

matrix3f 
mult_m3f(const matrix3f*, const matrix3f*);
void 
mult_set_m3f_f(matrix3f*, float);
void 
add_set_m3f(matrix3f*, const matrix3f*);

inline
void
matrix3f_set_axisangle(matrix3f* dst, const vector3f* axis, float alpha_degree)
{
  matrix3f s, ss, i;
  vector3f w = normalize_v3f(axis);
  float angle = alpha_degree / 180.f * (float)K_PI;

  matrix3f_set_identity(&s);
  matrix3f_set_identity(&ss);
  matrix3f_set_identity(&i);
  
  // s
  s.data[M3_RC_00] = s.data[M3_RC_11] = s.data[M3_RC_22] = 0.f;
  s.data[M3_RC_01] = -w.data[2];
  s.data[M3_RC_02] = w.data[1];
  s.data[M3_RC_10] = w.data[2];
  s.data[M3_RC_12] = -w.data[0];
  s.data[M3_RC_20] = -w.data[1];
  s.data[M3_RC_21] = w.data[0];
  s.data[M3_RC_22] = 0.f;
  ss = mult_m3f(&s, &s);
  
  // i + s * sinf(angle) + ss * (1.f - cosf(angle));
  mult_set_m3f_f(&s, sinf(angle));
  mult_set_m3f_f(&ss, (1.f - cosf(angle)));
  add_set_m3f(&i, &s);
  add_set_m3f(&i, &ss);
  matrix3f_copy(dst, &i);
}

////////////////////////////////////////////////////////////////////////////////
inline
float
determinant_m3f(const matrix3f* src)
{
  return 
    src->data[M3_RC_00] * src->data[M3_RC_11] * src->data[M3_RC_22] - 
    src->data[M3_RC_00] * src->data[M3_RC_12] * src->data[M3_RC_21] - 
    src->data[M3_RC_01] * src->data[M3_RC_10] * src->data[M3_RC_22] + 
    src->data[M3_RC_01] * src->data[M3_RC_12] * src->data[M3_RC_20] + 
    src->data[M3_RC_02] * src->data[M3_RC_10] * src->data[M3_RC_21] - 
    src->data[M3_RC_02] * src->data[M3_RC_11] * src->data[M3_RC_20];
}

/**
 * @brief will extract the axis and the angles in degrees from src. src should
 * be a rotation matrix (otherwise result is undefined).
 */
inline
void
to_axisangle_m3f(const matrix3f* src, vector3f* axis, float* angle_deg)
{
  float trace = src->data[M3_RC_00] + src->data[M3_RC_11] + src->data[M3_RC_22];
  *angle_deg = acosf((trace - 1.f) / 2.f);
  *angle_deg = *angle_deg / (float)K_PI * 180.f;
  
  if (K_EQUAL_TO(*angle_deg, 0.f, (2 * FLT_MIN)))
    vector3f_set_3f(axis, 0.f, 0.f, 1.f);
  else if (K_EQUAL_TO(*angle_deg, 180.f, (2 * FLT_MIN))) {
    float x = 0.f, y = 0.f, z = 0.f;
    if (
      (src->data[M3_RC_00] >= src->data[M3_RC_11]) && 
      (src->data[M3_RC_00] >= src->data[M3_RC_22])) {
      x = 
        sqrtf(src->data[M3_RC_00] - src->data[M3_RC_11] - 
        src->data[M3_RC_22] + 1.f) / 2.f;
      y = src->data[M3_RC_01] / (2 * x);
      z = src->data[M3_RC_02] / (2 * x);
    } else if (
      (src->data[M3_RC_11] >= src->data[M3_RC_00]) && 
      (src->data[M3_RC_11] >= src->data[M3_RC_22])) {
      y = 
        sqrtf(src->data[M3_RC_11] - src->data[M3_RC_00] - 
        src->data[M3_RC_22] + 1.f) / 2.f;
      x = src->data[M3_RC_01] / (2 * y);
      z = src->data[M3_RC_12] / (2 * y);
    } else if (
      (src->data[M3_RC_22] >= src->data[M3_RC_00]) && 
      (src->data[M3_RC_22] >= src->data[M3_RC_11])) {
      z = 
        sqrtf(src->data[M3_RC_22] - src->data[M3_RC_00] - 
        src->data[M3_RC_11] + 1.f) / 2.f;
      x = src->data[M3_RC_02] / (2 * z);
      y = src->data[M3_RC_12] / (2 * z);
    }

    axis->data[0] = x;
    axis->data[1] = y;
    axis->data[2] = z;
  } else {
    axis->data[0] = src->data[M3_RC_21] - src->data[M3_RC_12];
    axis->data[1] = src->data[M3_RC_02] - src->data[M3_RC_20];
    axis->data[2] = src->data[M3_RC_10] - src->data[M3_RC_01];
    normalize_set_v3f(axis);
  }
}

////////////////////////////////////////////////////////////////////////////////
inline
matrix3f
mult_m3f(const matrix3f* lhs, const matrix3f* rhs)
{
  matrix3f result;
  result.data[M3_RC_00] = 
    lhs->data[M3_RC_00] * rhs->data[M3_RC_00] + 
    lhs->data[M3_RC_01] * rhs->data[M3_RC_10] + 
    lhs->data[M3_RC_02] * rhs->data[M3_RC_20];
  result.data[M3_RC_01] = 
    lhs->data[M3_RC_00] * rhs->data[M3_RC_01] + 
    lhs->data[M3_RC_01] * rhs->data[M3_RC_11] + 
    lhs->data[M3_RC_02] * rhs->data[M3_RC_21];
  result.data[M3_RC_02] = 
    lhs->data[M3_RC_00] * rhs->data[M3_RC_02] + 
    lhs->data[M3_RC_01] * rhs->data[M3_RC_12] + 
    lhs->data[M3_RC_02] * rhs->data[M3_RC_22];
  result.data[M3_RC_10] = 
    lhs->data[M3_RC_10] * rhs->data[M3_RC_00] + 
    lhs->data[M3_RC_11] * rhs->data[M3_RC_10] + 
    lhs->data[M3_RC_12] * rhs->data[M3_RC_20];
  result.data[M3_RC_11] = 
    lhs->data[M3_RC_10] * rhs->data[M3_RC_01] + 
    lhs->data[M3_RC_11] * rhs->data[M3_RC_11] + 
    lhs->data[M3_RC_12] * rhs->data[M3_RC_21];
  result.data[M3_RC_12] = 
    lhs->data[M3_RC_10] * rhs->data[M3_RC_02] + 
    lhs->data[M3_RC_11] * rhs->data[M3_RC_12] + 
    lhs->data[M3_RC_12] * rhs->data[M3_RC_22];
  result.data[M3_RC_20] = 
    lhs->data[M3_RC_20] * rhs->data[M3_RC_00] + 
    lhs->data[M3_RC_21] * rhs->data[M3_RC_10] + 
    lhs->data[M3_RC_22] * rhs->data[M3_RC_20];
  result.data[M3_RC_21] = 
    lhs->data[M3_RC_20] * rhs->data[M3_RC_01] + 
    lhs->data[M3_RC_21] * rhs->data[M3_RC_11] + 
    lhs->data[M3_RC_22] * rhs->data[M3_RC_21];
  result.data[M3_RC_22] = 
    lhs->data[M3_RC_20] * rhs->data[M3_RC_02] + 
    lhs->data[M3_RC_21] * rhs->data[M3_RC_12] + 
    lhs->data[M3_RC_22] * rhs->data[M3_RC_22];
  return result;
}

inline
void
mult_set_m3f(matrix3f* dst, const matrix3f* rhs)
{
  matrix3f result = mult_m3f(dst, rhs);
  matrix3f_copy(dst, &result);
}

inline
matrix3f
mult_m3f_f(const matrix3f* lhs, float scale)
{
  matrix3f copy;
  matrix3f_copy(&copy, lhs);
  for (int32_t i = 0; i < 9; ++i)
    copy.data[i] *= scale;
  return copy;
}

inline
void
mult_set_m3f_f(matrix3f* dst, float scale)
{
  for (uint32_t i = 0; i < 9; ++i)
    dst->data[i] *= scale;
}

inline
matrix3f
add_m3f(const matrix3f* lhs, const matrix3f* rhs)
{
  matrix3f result;
  for (uint32_t i = 0; i < 9; ++i)
    result.data[i] = lhs->data[i] + rhs->data[i];
  return result;
}

inline
void
add_set_m3f(matrix3f* dst, const matrix3f* rhs)
{
  for (uint32_t i = 0; i < 9; ++i)
    dst->data[i] += rhs->data[i];
}

inline
vector3f
mult_m3f_vec3f(const matrix3f* lhs, const vector3f* rhs)
{
  vector3f result;
  result.data[0] = lhs->data[M3_RC_00] * rhs->data[0] + lhs->data[M3_RC_01] * rhs->data[1] + lhs->data[M3_RC_02] * rhs->data[2];
  result.data[1] = lhs->data[M3_RC_10] * rhs->data[0] + lhs->data[M3_RC_11] * rhs->data[1] + lhs->data[M3_RC_12] * rhs->data[2];
  result.data[2] = lhs->data[M3_RC_20] * rhs->data[0] + lhs->data[M3_RC_21] * rhs->data[1] + lhs->data[M3_RC_22] * rhs->data[2];
  return result;
}

/// @brief will return rhs address so it can be reused. 
inline
vector3f*
mult_set_m3f_vec3f(const matrix3f* lhs, vector3f* rhs)
{
  vector3f result = mult_m3f_vec3f(lhs, rhs);
  vector3f_copy(rhs, &result);
  return rhs;
}


#ifdef __cplusplus
}
#endif

#endif