/**
 * @file vector3f.h
 * @author khalilhenoud@gmail.com
 * @brief float 3 container, the interpretation of which is context dependent
 * @version 0.1
 * @date 2023-01-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef C_VECTOR_3F_H
#define C_VECTOR_3F_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <float.h>
#include <math/c/common.h>


typedef
struct {
  float data[3];
} vector3f;

typedef vector3f point3f;


////////////////////////////////////////////////////////////////////////////////
inline
void 
vector3f_set_1f(vector3f* dst, float value)
{
  dst->data[0] = dst->data[1] = dst->data[2] = value;
}

inline
void
vector3f_set_3f(vector3f* dst, float x, float y, float z)
{
  dst->data[0] = x;
  dst->data[1] = y;
  dst->data[2] = z;
}

inline
void
vector3f_copy(vector3f* dst, const vector3f* src)
{
  dst->data[0] = src->data[0];
  dst->data[1] = src->data[1];
  dst->data[2] = src->data[2];
}

inline
void
vector3f_set_a3f(vector3f* dst, const float* data)
{
  dst->data[0] = data[0];
  dst->data[1] = data[1];
  dst->data[2] = data[2]; 
}

inline
void
vector3f_set_diff_v3f(vector3f* dst, const vector3f* lhs, const vector3f* rhs)
{
  dst->data[0] = rhs->data[0] - lhs->data[0];
  dst->data[1] = rhs->data[1] - lhs->data[1];
  dst->data[2] = rhs->data[2] - lhs->data[2];
}

////////////////////////////////////////////////////////////////////////////////
inline
float
length_v3f(const vector3f* src)
{
  return sqrtf(
    src->data[0] * src->data[0] +
    src->data[1] * src->data[1] +
    src->data[2] * src->data[2]);
}

inline
float
length_squared_v3f(const vector3f* src)
{
  return 
    src->data[0] * src->data[0] + 
    src->data[1] * src->data[1] + 
    src->data[2] * src->data[2];
}

inline
float 
dot_product_v3f(const vector3f* lhs, const vector3f* rhs)
{
  return 
    lhs->data[0] * rhs->data[0] + 
    lhs->data[1] * rhs->data[1] + 
    lhs->data[2] * rhs->data[2];
}

inline
vector3f
cross_product_v3f(const vector3f* lhs, const vector3f* rhs)
{
  vector3f dst;
  dst.data[0] = lhs->data[1] * rhs->data[2] - rhs->data[1] * lhs->data[2];
  dst.data[1] = rhs->data[0] * lhs->data[2] - lhs->data[0] * rhs->data[2];
  dst.data[2] = lhs->data[0] * rhs->data[1] - rhs->data[0] * lhs->data[1];
  return dst;
}

inline
vector3f
normalize_v3f(const vector3f* src)
{
  float length = length_v3f(src);
  vector3f vec;
  vec.data[0] = src->data[0] / length;
  vec.data[1] = src->data[1] / length;
  vec.data[2] = src->data[2] / length;
  return vec;
}

inline
void
normalize_set_v3f(vector3f* dst)
{
  float length = length_v3f(dst);
  dst->data[0] /= length;
  dst->data[1] /= length;
  dst->data[2] /= length;
}

inline
int32_t
equal_to_v3f(const vector3f* lhs, const vector3f* rhs)
{
  vector3f vec;
  vector3f_set_diff_v3f(&vec, lhs, rhs);
  return nextafterf(length_squared_v3f(&vec), 0.f) == 0.f;
}

////////////////////////////////////////////////////////////////////////////////
inline
vector3f
negate_v3f(const vector3f* src)
{
  vector3f dst;
  dst.data[0] = -src->data[0];
  dst.data[1] = -src->data[1];
  dst.data[2] = -src->data[2];
  return dst;
}

inline
void
negate_set_v3f(vector3f* dst)
{
  dst->data[0] *= -1.f;
  dst->data[1] *= -1.f;
  dst->data[2] *= -1.f;
}

inline
vector3f
diff_v3f(const vector3f* lhs, const vector3f* rhs)
{
  vector3f dst;
  dst.data[0] = rhs->data[0] - lhs->data[0];
  dst.data[1] = rhs->data[1] - lhs->data[1];
  dst.data[2] = rhs->data[2] - lhs->data[2];
  return dst;
}

inline
void
diff_set_v3f(vector3f* dst, const vector3f* rhs)
{
  dst->data[0] -= rhs->data[0];
  dst->data[1] -= rhs->data[1];
  dst->data[2] -= rhs->data[2];
}

inline
vector3f
add_v3f(const vector3f* lhs, const vector3f* rhs)
{
  vector3f dst;
  dst.data[0] = lhs->data[0] + rhs->data[0];
  dst.data[1] = lhs->data[1] + rhs->data[1];
  dst.data[2] = lhs->data[2] + rhs->data[2];
  return dst;
}

inline
void
add_set_v3f(vector3f* dst, const vector3f* add)
{
  dst->data[0] += add->data[0];
  dst->data[1] += add->data[1];
  dst->data[2] += add->data[2];
}

inline
vector3f
mult_v3f(const vector3f* lhs, float scale)
{
  vector3f dst;
  dst.data[0] = lhs->data[0] * scale;
  dst.data[1] = lhs->data[1] * scale;
  dst.data[2] = lhs->data[2] * scale;
  return dst;
}

inline
void
mult_set_v3f(vector3f* dst, float scale)
{
  dst->data[0] *= scale;
  dst->data[1] *= scale;
  dst->data[2] *= scale;
}

inline
vector3f
div_v3f(const vector3f* lhs, float scale)
{
  vector3f dst;
  dst.data[0] = lhs->data[0] / scale;
  dst.data[1] = lhs->data[1] / scale;
  dst.data[2] = lhs->data[2] / scale;
  return dst;
}

inline
void
div_set_v3f(vector3f* dst, float scale)
{
  dst->data[0] /= scale;
  dst->data[1] /= scale;
  dst->data[2] /= scale;
}


#ifdef __cplusplus
}
#endif

#endif