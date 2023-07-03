/**
 * @file quat.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef C_QUAT_H
#define C_QUAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <math/c/vector3f.h>
#include <math/c/matrix3f.h>
#include <math/c/matrix4f.h>


typedef
enum {
  S, 
  X, 
  Y, 
  Z
} QUAT_DATA;

typedef
struct quatf {
  float data[4];
} quatf;


////////////////////////////////////////////////////////////////////////////////
inline
void
quatf_set_4f(quatf* dst, float s, float x, float y, float z)
{
  dst->data[S] = s;
  dst->data[X] = x;
  dst->data[Y] = y;
  dst->data[Z] = z;
}

inline
void
quatf_set_identity(quatf* dst)
{
  quat_set_4f(dst, 1.f, 0.f, 0.f, 0.f);
}

inline
void
quatf_set_a4f(quatf* dst, const float *data)
{
  quatf_set_4f(dst, data[0], data[1], data[2], data[3]);
}

inline
void
quatf_set_from_axis_angle(quatf* dst, const vector3f* axis, float angle_radian)
{
  float half_angle = angle_radian / 2.f;
  float sin_half = sinf(half_angle);
  dst->data[S] = cosf(half_angle);
  dst->data[X] = sin_half * axis->data[0];
  dst->data[Y] = sin_half * axis->data[1];
  dst->data[Z] = sin_half * axis->data[2];
}

// NOTE: 'from' is a rotation matrix with no scaling applied. 
// below is a very useful resource, it also contains an explanation as to why we
// test (trace > 0) as opposed to (trace + 1 > 0).
// https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
inline
void
quatf_set_from_matrix3f(quatf* dst, const matrix3f* from)
{
  float trace = from->data[M3_RC_00] + from->data[M3_RC_11] + from->data[M3_RC_22];
	if (trace > 0.f) {
		dst->data[S] = sqrtf(trace + 1.f) / 2.f;
		dst->data[X] = (from->data[M3_RC_12] - from->data[M3_RC_21])/(4 * dst->data[S]);
		dst->data[Y] = (from->data[M3_RC_20] - from->data[M3_RC_02])/(4 * dst->data[S]);
		dst->data[Z] = (from->data[M3_RC_01] - from->data[M3_RC_10])/(4 * dst->data[S]);
	} else {
    if (
      (from->data[M3_RC_00] >= from->data[M3_RC_11]) && 
      (from->data[M3_RC_00] >= from->data[M3_RC_22])) {
      dst->data[X] = sqrtf(from->data[M3_RC_00] - from->data[M3_RC_11] - from->data[M3_RC_22] + 1.f) / 2.f;
      dst->data[S] = (from->data[M3_RC_12] - from->data[M3_RC_21])/(4 * dst->data[X]);		
			dst->data[Y] = (from->data[M3_RC_01] + from->data[M3_RC_10])/(4 * dst->data[X]);
			dst->data[Z] = (from->data[M3_RC_02] + from->data[M3_RC_20])/(4 * dst->data[X]);
		} else if (
      (from->data[M3_RC_11] >= from->data[M3_RC_00]) && 
      (from->data[M3_RC_11] >= from->data[M3_RC_22])) {
      dst->data[Y] = sqrtf(from->data[M3_RC_11] - from->data[M3_RC_00] - from->data[M3_RC_22] + 1.f) / 2.f;
      dst->data[S] = (from->data[M3_RC_20] - from->data[M3_RC_02])/(4 * dst->data[Y]);
			dst->data[X] = (from->data[M3_RC_01] + from->data[M3_RC_10])/(4 * dst->data[Y]);
			dst->data[Z] = (from->data[M3_RC_12] + from->data[M3_RC_21])/(4 * dst->data[Y]);
		} else if (
      (from->data[M3_RC_22] >= from->data[M3_RC_00]) && 
      (from->data[M3_RC_22] >= from->data[M3_RC_11])) {
      dst->data[Z] = sqrtf(from->data[M3_RC_22] - from->data[M3_RC_00] - from->data[M3_RC_11] + 1.f) / 2.f;
      dst->data[S] = (from->data[M3_RC_01] - from->data[M3_RC_10])/(4 * dst->data[Z]);
			dst->data[X] = (from->data[M3_RC_02] + from->data[M3_RC_20])/(4 * dst->data[Z]);
			dst->data[Y] = (from->data[M3_RC_12] + from->data[M3_RC_21])/(4 * dst->data[Z]);
		}
  }
}

// @see quatf_set_from_matrix3f.
inline
void
quatf_set_from_matrix4f(quatf* dst, const matrix4f* from)
{
  float trace = from->data[M4_RC_00] + from->data[M4_RC_11] + from->data[M4_RC_22];
	if (trace > 0.f) {
		dst->data[S] = sqrtf(trace + 1.f) / 2.f;
		dst->data[X] = (from->data[M4_RC_12] - from->data[M4_RC_21])/(4 * dst->data[S]);
		dst->data[Y] = (from->data[M4_RC_20] - from->data[M4_RC_02])/(4 * dst->data[S]);
		dst->data[Z] = (from->data[M4_RC_01] - from->data[M4_RC_10])/(4 * dst->data[S]);
	} else {
    if (
      (from->data[M4_RC_00] >= from->data[M4_RC_11]) && 
      (from->data[M4_RC_00] >= from->data[M4_RC_22])) {
      dst->data[X] = sqrtf(from->data[M4_RC_00] - from->data[M4_RC_11] - from->data[M4_RC_22] + 1.f) / 2.f;
      dst->data[S] = (from->data[M4_RC_12] - from->data[M4_RC_21])/(4 * dst->data[X]);		
			dst->data[Y] = (from->data[M4_RC_01] + from->data[M4_RC_10])/(4 * dst->data[X]);
			dst->data[Z] = (from->data[M4_RC_02] + from->data[M4_RC_20])/(4 * dst->data[X]);
		} else if (
      (from->data[M4_RC_11] >= from->data[M4_RC_00]) && 
      (from->data[M4_RC_11] >= from->data[M4_RC_22])) {
      dst->data[Y] = sqrtf(from->data[M4_RC_11] - from->data[M4_RC_00] - from->data[M4_RC_22] + 1.f) / 2.f;
      dst->data[S] = (from->data[M4_RC_20] - from->data[M4_RC_02])/(4 * dst->data[Y]);
			dst->data[X] = (from->data[M4_RC_01] + from->data[M4_RC_10])/(4 * dst->data[Y]);
			dst->data[Z] = (from->data[M4_RC_12] + from->data[M4_RC_21])/(4 * dst->data[Y]);
		} else if (
      (from->data[M4_RC_22] >= from->data[M4_RC_00]) && 
      (from->data[M4_RC_22] >= from->data[M4_RC_11])) {
      dst->data[Z] = sqrtf(from->data[M4_RC_22] - from->data[M4_RC_00] - from->data[M4_RC_11] + 1.f) / 2.f;
      dst->data[S] = (from->data[M4_RC_01] - from->data[M4_RC_10])/(4 * dst->data[Z]);
			dst->data[X] = (from->data[M4_RC_02] + from->data[M4_RC_20])/(4 * dst->data[Z]);
			dst->data[Y] = (from->data[M4_RC_12] + from->data[M4_RC_21])/(4 * dst->data[Z]);
		}
  }
}

////////////////////////////////////////////////////////////////////////////////
inline
void
get_quatf_axis_angle(const quatf* src, vector3f* axis, float* angle_radian)
{
  if (nextafterf(fabs(src->data[S]), 1.f) == 1.f) {
    // no rotation.
    *angle_radian = 0.f;
    vector3f_set_3f(axis, 0.f, 0.f, 1.f);
  } else {
    float denom = sqrtf(1.f - src->data[S] * src->Data[S]);
    *angle_radian = 2 * acosf(src->data[S]);
    axis->x = src->data[X] / denom;
    axis->y = src->data[Y] / denom;
    axis->z = src->data[Z] / denom;
  }
}

////////////////////////////////////////////////////////////////////////////////
inline
float
length_quatf(const quatf* src)
{
  return sqrtf(
    src->data[S] * src->data[S] + 
    src->data[X] * src->data[X] + 
    src->data[Y] * src->data[Y] + 
    src->data[Z] * src->data[Z]);
}

inline
float
length_squared_quatf(const quatf* src)
{
  return 
    src->data[S] * src->data[S] + 
    src->data[X] * src->data[X] + 
    src->data[Y] * src->data[Y] + 
    src->data[Z] * src->data[Z];
}

inline
float
dot_product_quatf(const quatf* lhs, const quatf* rhs)
{
  vector3f v1, v2;
  vector3f_set_3f(lhs->data[X], lhs->data[Y], lhs->data[Z]);
  vector3f_set_3f(rhs->data[X], rhs->data[Y], rhs->data[Z]);
  return lhs->data[S] * rhs->data[S] + dot_product_v3f(&v1, &v2);
}

////////////////////////////////////////////////////////////////////////////////
inline
quatf
mult_quatf_f(const quatf* src, float scale)
{
  quatf dst = *src;
  dst.data[S] *= scale;
  dst.data[X] *= scale;
  dst.data[Y] *= scale;
  dst.data[Z] *= scale;
  return dst;
}

inline
void
mult_set_quatf_f(quatf* dst, float scale)
{
  dst->data[S] *= scale;
  dst->data[X] *= scale;
  dst->data[Y] *= scale;
  dst->data[Z] *= scale;
}

inline
quatf
add_quatf(const quatf* lhs, const quatf* rhs)
{
  quatf dst = *lhs;
  for (uint32_t i = 0; i < 4; ++i)
    dst.data[i] += rhs->data[i];

  return dst;
}

inline
void
add_set_quatf(quatf* dst, const quatf* rhs)
{
  for (uint32_t i = 0; i < 4; ++i)
    dst.data[i] += rhs->data[i];
}

inline
quatf
mult_quatf(const quatf* lhs, const quatf* rhs)
{
  quatf result;
  vector3f v0, v1, v2;
  vector3f_set_a3f(&v1, lhs->data + X);
  vector3f_set_a3f(&v2, rhs->data + X);
  result.data[S] = lhs->data[S] * rhs->data[S] - dot_product_v3f(&v1, &v2);
  {
    // calc v0.
    vector3f vb, vc;
    v0 = mult_v3f(&v2, lhs->data[S]);
    vb = mult_v3f(&v1, rhs->data[S]);
    vc = cross_product_v3f(&v1, &v2);
    add_set_v3f(&v0, &vb);
    add_set_v3f(&v0, &vc);
  }
  result.data[X] = v0.data[0];
  result.data[Y] = v0.data[1];
  result.data[Z] = v0.data[2];
  return result;
}

inline
void
mult_set_quatf(quatf* dst, const quatf* rhs)
{
  quatf result = mult_quatf(dst, rhs);
  *dst = result;
}

inline
vector3f
mult_quatf_v3f(const quatf* quat, const vector3f* vec)
{
  quatf r, q, qinv, result;
  vector3f m;
  quatf_set_4f(&r, 0.f, vec.x, vec.y, vec.z);
  q = *quat;
  qinv = inverse_quatf(&q);
  result = mult_quatf(&q, &r);
  result = mult_quatf(&result, &qinv);
  vector3f_set_3f(&m, result.data[X], result.data[Y], result.data[Z]);
  return m;
}

////////////////////////////////////////////////////////////////////////////////
inline
quatf
inverse_quatf(const quatf* src)
{
  quatf result;
  float l = length_squared_quatf(src);
  result.data[S] = src->data[S] / l;
  result.data[X] = -src->data[X] / l;
  result.data[Y] = -src->data[Y] / l;
  result.data[Z] = -src->data[Z] / l;
  return result;
}

inline
void
inverse_set_quatf(quatf* dst)
{
  *dst = inverse_quatf(dst);
}

// NOTE: this would act as the inverse if src is a unitary quaternion.
inline
quatf
conjugate_quatf(const quatf* src)
{
  quatf result;
  result.data[S] = src->data[S];
  result.data[X] = -src->data[X];
  result.data[Y] = -src->data[Y];
  result.data[Z] = -src->data[Z];
  return result;
}

inline
void
conjugate_set_quatf(quatf* dst)
{
  *dst = conjugate_quatf(dst);
}

#ifdef __cplusplus
}
#endif

#endif