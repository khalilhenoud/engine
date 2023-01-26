/**
 * @file matrix4f.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef C_MATRIX4F_H
#define C_MATRIX4F_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math/c/matrix3f.h>


typedef
enum {
  M4_RC_00,
  M4_RC_01,
  M4_RC_02,
  M4_RC_03,
  M4_RC_10,
  M4_RC_11,
  M4_RC_12,
  M4_RC_13,
  M4_RC_20,
  M4_RC_21,
  M4_RC_22,
  M4_RC_23,
  M4_RC_30,
  M4_RC_31,
  M4_RC_32,
  M4_RC_33
} M4_RC_XX;

typedef 
struct {
  float data[16];
} matrix4f;


////////////////////////////////////////////////////////////////////////////////
inline
void
matrix4f_set_identity(matrix4f* dst)
{
  dst->data[M4_RC_01] = dst->data[M4_RC_02] = dst->data[M4_RC_03] = dst->data[M4_RC_10] = 
  dst->data[M4_RC_12] = dst->data[M4_RC_13] = dst->data[M4_RC_20] = dst->data[M4_RC_21] = 
  dst->data[M4_RC_23] = dst->data[M4_RC_30] = dst->data[M4_RC_31] = dst->data[M4_RC_32] = 0.f;
  dst->data[M4_RC_00] = dst->data[M4_RC_11] = dst->data[M4_RC_22] = dst->data[M4_RC_33] = 1.f;
}

inline
void
matrix4f_copy(matrix4f* dst, const matrix4f* src)
{
  memcpy(dst->data, src->data, sizeof(dst->data));
}

inline
void
matrix4f_rotation_x(matrix4f* dst, float angle_radian)
{
  matrix4f_set_identity(dst);
  dst->data[M4_RC_11] = dst->data[M4_RC_22] = cosf(angle_radian);
  dst->data[M4_RC_21] = sinf(angle_radian);
  dst->data[M4_RC_12] = -dst->data[M4_RC_21];
}

inline
void
matrix4f_rotation_y(matrix4f* dst, float angle_radian)
{
  matrix4f_set_identity(dst);
  dst->data[M4_RC_00] = dst->data[M4_RC_22] = cosf(angle_radian);
  dst->data[M4_RC_02] = sinf(angle_radian);
  dst->data[M4_RC_20] = -dst->data[M4_RC_02];
}

inline
void
matrix4f_rotation_z(matrix4f* dst, float angle_radian)
{
  matrix4f_set_identity(dst);
  dst->data[M4_RC_00] = dst->data[M4_RC_11] = cosf(angle_radian);
  dst->data[M4_RC_10] = sinf(angle_radian);
  dst->data[M4_RC_01] = -dst->data[M4_RC_10];
}

inline
void
matrix4f_translation(matrix4f* dst, float x, float y, float z)
{
  matrix4f_set_identity(dst);
  dst->data[M4_RC_03] = x;
  dst->data[M4_RC_13] = y;
  dst->data[M4_RC_23] = z;
}

inline
void
matrix4f_scale(matrix4f* dst, float x, float y, float z)
{
  matrix4f_set_identity(dst);
  dst->data[M4_RC_00] = x;
  dst->data[M4_RC_11] = y;
  dst->data[M4_RC_22] = z;
}

inline
void
matrix4f_set_axisangle(matrix4f* dst, const vector3f* axis, float alpha_degree)
{
  matrix3f tmp;
  matrix3f_set_axisangle(&tmp, axis, alpha_degree);
  dst->data[M4_RC_00] = tmp.data[M3_RC_00];	dst->data[M4_RC_01] = tmp.data[M3_RC_01];	dst->data[M4_RC_02] = tmp.data[M3_RC_02];	dst->data[M4_RC_03] = 0.f;
  dst->data[M4_RC_10] = tmp.data[M3_RC_10];	dst->data[M4_RC_11] = tmp.data[M3_RC_11];	dst->data[M4_RC_12] = tmp.data[M3_RC_12];	dst->data[M4_RC_13] = 0.f;
  dst->data[M4_RC_20] = tmp.data[M3_RC_20];	dst->data[M4_RC_21] = tmp.data[M3_RC_21];	dst->data[M4_RC_22] = tmp.data[M3_RC_22];	dst->data[M4_RC_23] = 0.f;
  dst->data[M4_RC_30] = 0.f;		            dst->data[M4_RC_31] = 0.f;		            dst->data[M4_RC_32] = 0.f;		            dst->data[M4_RC_33] = 1.f;
}

/// @brief Calculate the matrix that when multiplied by another vector 'v' will
/// give the equivalent @a vec cross 'v' resultant vector.
inline
void
matrix4f_cross_product(matrix4f* dst, const vector3f* vec)
{
  vector3f result = normalize_v3f(vec);
  matrix4f_set_identity(dst);

  dst->data[M4_RC_00] = dst->data[M4_RC_30] = dst->data[M4_RC_11] = 
  dst->data[M4_RC_31] = dst->data[M4_RC_22] = dst->data[M4_RC_32] = 0.f;
  dst->data[M4_RC_10] = result.data[2];
  dst->data[M4_RC_20] = -result.data[1];
  dst->data[M4_RC_01] = -result.data[2];
  dst->data[M4_RC_21] = result.data[0];
  dst->data[M4_RC_02] = result.data[1];
  dst->data[M4_RC_12] = -result.data[0];
}

/// @brief convert our row major matrix (directx format) to column major (opengl
/// is column major). 
inline
void
matrix4f_set_column_major(matrix4f* dst, const matrix4f* src)
{
  dst->data[0] = src->data[M4_RC_00];
  dst->data[1] = src->data[M4_RC_10];
  dst->data[2] = src->data[M4_RC_20];
  dst->data[3] = src->data[M4_RC_30];
  dst->data[4] = src->data[M4_RC_01];
  dst->data[5] = src->data[M4_RC_11];
  dst->data[6] = src->data[M4_RC_21];
  dst->data[7] = src->data[M4_RC_31];
  dst->data[8] = src->data[M4_RC_02];
  dst->data[9] = src->data[M4_RC_12];
  dst->data[10] = src->data[M4_RC_22];
  dst->data[11] = src->data[M4_RC_32];
  dst->data[12] = src->data[M4_RC_03];
  dst->data[13] = src->data[M4_RC_13];
  dst->data[14] = src->data[M4_RC_23];
  dst->data[15] = src->data[M4_RC_33];
}

////////////////////////////////////////////////////////////////////////////////
inline
float
determinant_m4f(const matrix4f* src)
{
  matrix3f d0, d1, d2, d3;
  float det0, det1, det2, det3;

  d0.data[M3_RC_00] = src->data[M4_RC_11]; d0.data[M3_RC_01] = src->data[M4_RC_12]; d0.data[M3_RC_02] = src->data[M4_RC_13];
  d0.data[M3_RC_10] = src->data[M4_RC_21]; d0.data[M3_RC_11] = src->data[M4_RC_22]; d0.data[M3_RC_12] = src->data[M4_RC_23];
  d0.data[M3_RC_20] = src->data[M4_RC_31]; d0.data[M3_RC_21] = src->data[M4_RC_32]; d0.data[M3_RC_22] = src->data[M4_RC_33];

  d1.data[M3_RC_00] = src->data[M4_RC_10]; d1.data[M3_RC_01] = src->data[M4_RC_12]; d1.data[M3_RC_02] = src->data[M4_RC_13];
  d1.data[M3_RC_10] = src->data[M4_RC_20]; d1.data[M3_RC_11] = src->data[M4_RC_22]; d1.data[M3_RC_12] = src->data[M4_RC_23];
  d1.data[M3_RC_20] = src->data[M4_RC_30]; d1.data[M3_RC_21] = src->data[M4_RC_32]; d1.data[M3_RC_22] = src->data[M4_RC_33];

  d2.data[M3_RC_00] = src->data[M4_RC_10]; d2.data[M3_RC_01] = src->data[M4_RC_11]; d2.data[M3_RC_02] = src->data[M4_RC_13];
  d2.data[M3_RC_10] = src->data[M4_RC_20]; d2.data[M3_RC_11] = src->data[M4_RC_21]; d2.data[M3_RC_12] = src->data[M4_RC_23];
  d2.data[M3_RC_20] = src->data[M4_RC_30]; d2.data[M3_RC_21] = src->data[M4_RC_31]; d2.data[M3_RC_22] = src->data[M4_RC_33];

  d3.data[M3_RC_00] = src->data[M4_RC_10]; d3.data[M3_RC_01] = src->data[M4_RC_11]; d3.data[M3_RC_02] = src->data[M4_RC_12];
  d3.data[M3_RC_10] = src->data[M4_RC_20]; d3.data[M3_RC_11] = src->data[M4_RC_21]; d3.data[M3_RC_12] = src->data[M4_RC_22];
  d3.data[M3_RC_20] = src->data[M4_RC_30]; d3.data[M3_RC_21] = src->data[M4_RC_31]; d3.data[M3_RC_22] = src->data[M4_RC_32];

  det0 = determinant_m3f(&d0);
  det1 = determinant_m3f(&d1);
  det2 = determinant_m3f(&d2);
  det3 = determinant_m3f(&d3);

  return 
    src->data[M4_RC_00] * det0 - src->data[M4_RC_01] * det1 + 
    src->data[M4_RC_02] * det2 - src->data[M4_RC_03] * det3;
}

inline
matrix4f
transpose_m4f(const matrix4f* src)
{
  matrix4f result;
  result.data[M4_RC_00] = src->data[M4_RC_00];	result.data[M4_RC_01] = src->data[M4_RC_10];	result.data[M4_RC_02] = src->data[M4_RC_20];	result.data[M4_RC_03] = src->data[M4_RC_30];
  result.data[M4_RC_10] = src->data[M4_RC_01];	result.data[M4_RC_11] = src->data[M4_RC_11];	result.data[M4_RC_12] = src->data[M4_RC_21];	result.data[M4_RC_13] = src->data[M4_RC_31];
  result.data[M4_RC_20] = src->data[M4_RC_02];	result.data[M4_RC_21] = src->data[M4_RC_12];	result.data[M4_RC_22] = src->data[M4_RC_22];	result.data[M4_RC_23] = src->data[M4_RC_32];
  result.data[M4_RC_30] = src->data[M4_RC_03];	result.data[M4_RC_31] = src->data[M4_RC_13];	result.data[M4_RC_32] = src->data[M4_RC_23];	result.data[M4_RC_33] = src->data[M4_RC_33];
  return result;
}

inline
void
transpose_set_m4f(matrix4f* dst)
{
  matrix4f result = transpose_m4f(dst);
  matrix4f_copy(dst, &result);
}

inline
matrix4f 
inverse_m4f(const matrix4f* src)
{
  void mult_set_m4f_f(matrix4f*, float);
  float det1, det2, det3, det4, 
        det5, det6, det7, det8, 
        det9, det10, det11, det12, 
        det13, det14, det15, det16;
  matrix3f tmp;
  matrix4f result;

  tmp.data[M3_RC_00] = src->data[M4_RC_11]; tmp.data[M3_RC_01] = src->data[M4_RC_12]; tmp.data[M3_RC_02] = src->data[M4_RC_13];
  tmp.data[M3_RC_10] = src->data[M4_RC_21]; tmp.data[M3_RC_11] = src->data[M4_RC_22]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_31]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det1 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_10]; tmp.data[M3_RC_01] = src->data[M4_RC_12]; tmp.data[M3_RC_02] = src->data[M4_RC_13];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_22]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det2 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_10]; tmp.data[M3_RC_01] = src->data[M4_RC_11]; tmp.data[M3_RC_02] = src->data[M4_RC_13];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_21]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det3 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_10]; tmp.data[M3_RC_01] = src->data[M4_RC_11]; tmp.data[M3_RC_02] = src->data[M4_RC_12];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_21]; tmp.data[M3_RC_12] = src->data[M4_RC_22];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_32];
  det4 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_01]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_21]; tmp.data[M3_RC_11] = src->data[M4_RC_22]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_31]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det5 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_22]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det6 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_21]; tmp.data[M3_RC_12] = src->data[M4_RC_23];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det7 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_02];
  tmp.data[M3_RC_10] = src->data[M4_RC_20]; tmp.data[M3_RC_11] = src->data[M4_RC_21]; tmp.data[M3_RC_12] = src->data[M4_RC_22];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_32];
  det8 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_01]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_11]; tmp.data[M3_RC_11] = src->data[M4_RC_12]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_31]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det9 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_12]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_32]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det10 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_11]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_33];
  det11 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_02];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_11]; tmp.data[M3_RC_12] = src->data[M4_RC_12];
  tmp.data[M3_RC_20] = src->data[M4_RC_30]; tmp.data[M3_RC_21] = src->data[M4_RC_31]; tmp.data[M3_RC_22] = src->data[M4_RC_32];
  det12 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_01]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_11]; tmp.data[M3_RC_11] = src->data[M4_RC_12]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_21]; tmp.data[M3_RC_21] = src->data[M4_RC_22]; tmp.data[M3_RC_22] = src->data[M4_RC_23];
  det13 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_02]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_12]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_20]; tmp.data[M3_RC_21] = src->data[M4_RC_22]; tmp.data[M3_RC_22] = src->data[M4_RC_23];
  det14 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_03];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_11]; tmp.data[M3_RC_12] = src->data[M4_RC_13];
  tmp.data[M3_RC_20] = src->data[M4_RC_20]; tmp.data[M3_RC_21] = src->data[M4_RC_21]; tmp.data[M3_RC_22] = src->data[M4_RC_23];
  det15 = determinant_m3f(&tmp);

  tmp.data[M3_RC_00] = src->data[M4_RC_00]; tmp.data[M3_RC_01] = src->data[M4_RC_01]; tmp.data[M3_RC_02] = src->data[M4_RC_02];
  tmp.data[M3_RC_10] = src->data[M4_RC_10]; tmp.data[M3_RC_11] = src->data[M4_RC_11]; tmp.data[M3_RC_12] = src->data[M4_RC_12];
  tmp.data[M3_RC_20] = src->data[M4_RC_20]; tmp.data[M3_RC_21] = src->data[M4_RC_21]; tmp.data[M3_RC_22] = src->data[M4_RC_22];
  det16 = determinant_m3f(&tmp);

  result.data[M4_RC_00] = det1;			result.data[M4_RC_01] = -det2;		result.data[M4_RC_02] = det3;			result.data[M4_RC_03] = -det4;
  result.data[M4_RC_10] = -det5;		result.data[M4_RC_11] = det6;			result.data[M4_RC_12] = -det7;		result.data[M4_RC_13] = det8;
  result.data[M4_RC_20] = det9;			result.data[M4_RC_21] = -det10;		result.data[M4_RC_22] = det11;		result.data[M4_RC_23] = -det12;
  result.data[M4_RC_30] = -det13;		result.data[M4_RC_31] = det14;		result.data[M4_RC_32] = -det15;		result.data[M4_RC_33] = det16;

  transpose_set_m4f(&result);
  mult_set_m4f_f(&result, (1 / determinant_m4f(src)));
  return result;
}

inline
void
inverse_set_m4f(matrix4f* dst)
{
  matrix4f result = inverse_m4f(dst);
  matrix4f_copy(dst, &result);
}

/**
 * @brief will extract the axis and the angles in degrees from src. src should
 * be a rotation matrix (otherwise result is undefined).
 */
inline
void
to_axisangle_m4f(const matrix4f* src, vector3f* axis, float* angle_deg)
{
  float trace = src->data[M4_RC_00] + src->data[M4_RC_11] + src->data[M4_RC_22];
  *angle_deg = acosf((trace - 1.f) / 2.f);
  *angle_deg = *angle_deg / (float)K_PI * 180.f;
  
  if (K_EQUAL_TO(*angle_deg, 0.f, (2 * FLT_MIN)))
    vector3f_set_3f(axis, 0.f, 0.f, 1.f);
  else if (K_EQUAL_TO(*angle_deg, 180.f, (2 * FLT_MIN))) {
    float x = 0.f, y = 0.f, z = 0.f;
    if (
      (src->data[M4_RC_00] >= src->data[M4_RC_11]) && 
      (src->data[M4_RC_00] >= src->data[M4_RC_22])) {
      x = 
        sqrtf(src->data[M4_RC_00] - src->data[M4_RC_11] - 
        src->data[M4_RC_22] + 1.f) / 2.f;
      y = src->data[M4_RC_01] / (2 * x);
      z = src->data[M4_RC_02] / (2 * x);
    } else if (
      (src->data[M4_RC_11] >= src->data[M4_RC_00]) && 
      (src->data[M4_RC_11] >= src->data[M4_RC_22])) {
      y = 
        sqrtf(src->data[M4_RC_11] - src->data[M4_RC_00] - 
        src->data[M4_RC_22] + 1.f) / 2.f;
      x = src->data[M4_RC_01] / (2 * y);
      z = src->data[M4_RC_12] / (2 * y);
    } else if (
      (src->data[M4_RC_22] >= src->data[M4_RC_00]) && 
      (src->data[M4_RC_22] >= src->data[M4_RC_11])) {
      z = 
        sqrtf(src->data[M4_RC_22] - src->data[M4_RC_00] - 
        src->data[M4_RC_11] + 1.f) / 2.f;
      x = src->data[M4_RC_02] / (2 * z);
      y = src->data[M4_RC_12] / (2 * z);
    }

    axis->data[0] = x;
    axis->data[1] = y;
    axis->data[2] = z;
  } else {
    axis->data[0] = src->data[M4_RC_21] - src->data[M4_RC_12];
    axis->data[1] = src->data[M4_RC_02] - src->data[M4_RC_20];
    axis->data[2] = src->data[M4_RC_10] - src->data[M4_RC_01];
    normalize_set_v3f(axis);
  }
}

////////////////////////////////////////////////////////////////////////////////
inline
matrix4f
mult_m4f(const matrix4f* lhs, const matrix4f* rhs)
{
  matrix4f result;
  result.data[M4_RC_00] = lhs->data[M4_RC_00] * rhs->data[M4_RC_00] + 
                          lhs->data[M4_RC_01] * rhs->data[M4_RC_10] + 
                          lhs->data[M4_RC_02] * rhs->data[M4_RC_20] + 
                          lhs->data[M4_RC_03] * rhs->data[M4_RC_30];
  result.data[M4_RC_01] = lhs->data[M4_RC_00] * rhs->data[M4_RC_01] + 
                          lhs->data[M4_RC_01] * rhs->data[M4_RC_11] + 
                          lhs->data[M4_RC_02] * rhs->data[M4_RC_21] + 
                          lhs->data[M4_RC_03] * rhs->data[M4_RC_31];
  result.data[M4_RC_02] = lhs->data[M4_RC_00] * rhs->data[M4_RC_02] + 
                          lhs->data[M4_RC_01] * rhs->data[M4_RC_12] + 
                          lhs->data[M4_RC_02] * rhs->data[M4_RC_22] + 
                          lhs->data[M4_RC_03] * rhs->data[M4_RC_32];
  result.data[M4_RC_03] = lhs->data[M4_RC_00] * rhs->data[M4_RC_03] + 
                          lhs->data[M4_RC_01] * rhs->data[M4_RC_13] + 
                          lhs->data[M4_RC_02] * rhs->data[M4_RC_23] + 
                          lhs->data[M4_RC_03] * rhs->data[M4_RC_33];
  result.data[M4_RC_10] = lhs->data[M4_RC_10] * rhs->data[M4_RC_00] + 
                          lhs->data[M4_RC_11] * rhs->data[M4_RC_10] + 
                          lhs->data[M4_RC_12] * rhs->data[M4_RC_20] + 
                          lhs->data[M4_RC_13] * rhs->data[M4_RC_30];
  result.data[M4_RC_11] = lhs->data[M4_RC_10] * rhs->data[M4_RC_01] + 
                          lhs->data[M4_RC_11] * rhs->data[M4_RC_11] + 
                          lhs->data[M4_RC_12] * rhs->data[M4_RC_21] + 
                          lhs->data[M4_RC_13] * rhs->data[M4_RC_31];
  result.data[M4_RC_12] = lhs->data[M4_RC_10] * rhs->data[M4_RC_02] + 
                          lhs->data[M4_RC_11] * rhs->data[M4_RC_12] + 
                          lhs->data[M4_RC_12] * rhs->data[M4_RC_22] + 
                          lhs->data[M4_RC_13] * rhs->data[M4_RC_32];
  result.data[M4_RC_13] = lhs->data[M4_RC_10] * rhs->data[M4_RC_03] + 
                          lhs->data[M4_RC_11] * rhs->data[M4_RC_13] + 
                          lhs->data[M4_RC_12] * rhs->data[M4_RC_23] + 
                          lhs->data[M4_RC_13] * rhs->data[M4_RC_33];
  result.data[M4_RC_20] = lhs->data[M4_RC_20] * rhs->data[M4_RC_00] + 
                          lhs->data[M4_RC_21] * rhs->data[M4_RC_10] + 
                          lhs->data[M4_RC_22] * rhs->data[M4_RC_20] + 
                          lhs->data[M4_RC_23] * rhs->data[M4_RC_30];
  result.data[M4_RC_21] = lhs->data[M4_RC_20] * rhs->data[M4_RC_01] + 
                          lhs->data[M4_RC_21] * rhs->data[M4_RC_11] + 
                          lhs->data[M4_RC_22] * rhs->data[M4_RC_21] + 
                          lhs->data[M4_RC_23] * rhs->data[M4_RC_31];
  result.data[M4_RC_22] = lhs->data[M4_RC_20] * rhs->data[M4_RC_02] + 
                          lhs->data[M4_RC_21] * rhs->data[M4_RC_12] + 
                          lhs->data[M4_RC_22] * rhs->data[M4_RC_22] + 
                          lhs->data[M4_RC_23] * rhs->data[M4_RC_32];
  result.data[M4_RC_23] = lhs->data[M4_RC_20] * rhs->data[M4_RC_03] + 
                          lhs->data[M4_RC_21] * rhs->data[M4_RC_13] + 
                          lhs->data[M4_RC_22] * rhs->data[M4_RC_23] + 
                          lhs->data[M4_RC_23] * rhs->data[M4_RC_33];
  result.data[M4_RC_30] = lhs->data[M4_RC_30] * rhs->data[M4_RC_00] + 
                          lhs->data[M4_RC_31] * rhs->data[M4_RC_10] + 
                          lhs->data[M4_RC_32] * rhs->data[M4_RC_20] + 
                          lhs->data[M4_RC_33] * rhs->data[M4_RC_30];
  result.data[M4_RC_31] = lhs->data[M4_RC_30] * rhs->data[M4_RC_01] + 
                          lhs->data[M4_RC_31] * rhs->data[M4_RC_11] + 
                          lhs->data[M4_RC_32] * rhs->data[M4_RC_21] + 
                          lhs->data[M4_RC_33] * rhs->data[M4_RC_31];
  result.data[M4_RC_32] = lhs->data[M4_RC_30] * rhs->data[M4_RC_02] + 
                          lhs->data[M4_RC_31] * rhs->data[M4_RC_12] + 
                          lhs->data[M4_RC_32] * rhs->data[M4_RC_22] + 
                          lhs->data[M4_RC_33] * rhs->data[M4_RC_32];
  result.data[M4_RC_33] = lhs->data[M4_RC_30] * rhs->data[M4_RC_03] + 
                          lhs->data[M4_RC_31] * rhs->data[M4_RC_13] + 
                          lhs->data[M4_RC_32] * rhs->data[M4_RC_23] + 
                          lhs->data[M4_RC_33] * rhs->data[M4_RC_33];
  return result;
}

inline
void
mult_set_m4f(matrix4f* dst, const matrix4f* rhs)
{
  matrix4f result = mult_m4f(dst, rhs);
  matrix4f_copy(dst, &result);
}

inline
matrix4f
mult_m4f_f(const matrix4f* src, float scale)
{
  matrix4f result;
  matrix4f_copy(&result, src);
  for (uint32_t i = 0; i < 16; ++i)
    result.data[i] *= scale;
  return result;
}

inline
void
mult_set_m4f_f(matrix4f* dst, float scale)
{
  for (uint32_t i = 0; i < 16; ++i)
    dst->data[i] *= scale;
}

inline
vector3f
mult_m4f_v3f(const matrix4f* lhs, const vector3f* rhs)
{
  vector3f result;
  vector3f_set_3f(
    &result,
    lhs->data[M4_RC_00] * rhs->data[0] + lhs->data[M4_RC_01] * rhs->data[1] + lhs->data[M4_RC_02] * rhs->data[2],
    lhs->data[M4_RC_10] * rhs->data[0] + lhs->data[M4_RC_11] * rhs->data[1] + lhs->data[M4_RC_12] * rhs->data[2],
    lhs->data[M4_RC_20] * rhs->data[0] + lhs->data[M4_RC_21] * rhs->data[1] + lhs->data[M4_RC_22] * rhs->data[2]);
  return result;
}

/// @brief will return rhs address so it can be reused. 
inline
vector3f*
mult_set_m4f_v3f(const matrix4f* lhs, vector3f* rhs)
{
  vector3f result = mult_m4f_v3f(lhs, rhs);
  vector3f_copy(rhs, &result);  
  return rhs;
}

/// @brief point variant of the vector functionality, takes into account
/// translation transformation.
inline
vector3f
mult_m4f_p3f(const matrix4f* lhs, const vector3f* rhs)
{
  vector3f result;
  vector3f_set_3f(
    &result,
    lhs->data[M4_RC_00] * rhs->data[0] + lhs->data[M4_RC_01] * rhs->data[1] + lhs->data[M4_RC_02] * rhs->data[2] + lhs->data[M4_RC_03],
    lhs->data[M4_RC_10] * rhs->data[0] + lhs->data[M4_RC_11] * rhs->data[1] + lhs->data[M4_RC_12] * rhs->data[2] + lhs->data[M4_RC_13],
    lhs->data[M4_RC_20] * rhs->data[0] + lhs->data[M4_RC_21] * rhs->data[1] + lhs->data[M4_RC_22] * rhs->data[2] + lhs->data[M4_RC_23]);
  return result;
}

/// @brief point variant of the vector functionality, takes into account
/// translation transformation. Will return rhs address so it can be reused.
inline
vector3f*
mult_set_m4f_p3f(const matrix4f* lhs, vector3f* rhs)
{
  vector3f result = mult_m4f_p3f(lhs, rhs);
  vector3f_copy(rhs, &result);
  return rhs;
}


#ifdef __cplusplus
}
#endif

#endif