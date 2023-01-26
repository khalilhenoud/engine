/**
 * @file matrix4f.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CPP_MATRIX_4F_H
#define CPP_MATRIX_4F_H

#include <cmath>
#include <math/common.h>
#include <math/vector3f.h>
#include <math/matrix3f.h>
#include <math/c/matrix4f.h>


namespace math {

struct 
matrix4f {
  matrix4f(const ::matrix4f& m4)
  {
    ::matrix4f_copy(&data, &m4);
  }

  matrix4f()
  {
    ::matrix4f_set_identity(&data);
  }

  matrix4f(const vector3f& axis, float alpha_degrees)
  {
    ::matrix4f_set_axisangle(&data, &axis.data, alpha_degrees);
  }

  static
  matrix4f 
  axisangle(const vector3f& axis, float alpha_degrees)
  {
    return matrix4f(axis, alpha_degrees);
  }

  static 
  matrix4f 
  rotation_x(float angle_radian)
  {
    ::matrix4f result;
    ::matrix4f_rotation_x(&result, angle_radian);
    return result;
  }

  static 
  matrix4f 
  rotation_y(float angle_radian)
  {
    ::matrix4f result;
    ::matrix4f_rotation_y(&result, angle_radian);
    return result;
  }

  static 
  matrix4f 
  rotation_z(float angle_radian)
  {
    ::matrix4f result;
    ::matrix4f_rotation_z(&result, angle_radian);
    return result;
  }

  static 
  matrix4f 
  translation(float x, float y, float z)
  {
    ::matrix4f result;
    ::matrix4f_translation(&result, x, y, z);
    return result;
  }

  static 
  matrix4f 
  scale(float x, float y, float z)
  {
    ::matrix4f result;
    ::matrix4f_scale(&result, x, y, z);
    return result;
  }

  static 
  matrix4f 
  cross_product(const vector3f& vec)
  {
    ::matrix4f result;
    ::matrix4f_cross_product(&result, &vec.data);
    return result;
  }

  //////////////////////////////////////////////////////////////////////////////
  float 
  determinant(void) const
  {
    return ::determinant_m4f(&data);
  }

  matrix4f 
  inverse(void) const
  {
    return ::inverse_m4f(&data);
  }

  void 
  set_inverse(void)
  {
    ::inverse_set_m4f(&data);
  }

  matrix4f 
  transpose(void) const
  {
    return ::transpose_m4f(&data);
  }

  void 
  set_transpose(void)
  {
    ::transpose_set_m4f(&data);
  }

  matrix4f 
  to_column_major() const
  {
    ::matrix4f result;
    ::matrix4f_set_column_major(&result, &data);
    return result;
  }

  void 
  to_axisangle(vector3f& axis, float& angle) const
  {
    ::to_axisangle_m4f(&data, &axis.data, &angle);
  }

  //////////////////////////////////////////////////////////////////////////////
  matrix4f 
  operator *(float scale) const
  {
    return ::mult_m4f_f(&data, scale);
  }

  vector3f 
  mult_vec(const vector3f& vec3) const
  {
    return ::mult_m4f_v3f(&data, &vec3.data);
  }

  vector3f 
  mult_point(const vector3f& pt3) const
  {
    return ::mult_m4f_p3f(&data, &pt3.data);
  }

  matrix4f 
  operator *(const matrix4f& mtx) const
  {
    return ::mult_m4f(&data, &mtx.data);
  }

  constexpr float& 
  operator[](M4_RC_XX index)
  {
    return data.data[static_cast<int32_t>(index)];
  }

  constexpr const float& 
  operator[](M4_RC_XX index) const
  {
    return data.data[static_cast<int32_t>(index)];
  }

  ::matrix4f data;
};

inline
matrix4f 
operator *(float scale, const matrix4f& mtx)
{
  return ::mult_m4f_f(&mtx.data, scale);
}

}

#endif
