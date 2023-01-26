/**
 * @file matrix3f.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CPP_MATRIX3F_H
#define CPP_MATRIX3F_H

#include <math/common.h>
#include <math/vector3f.h>
#include <math/c/matrix3f.h>


namespace math {

struct 
matrix3f {
  matrix3f(const ::matrix3f& m3)
  {
    ::matrix3f_copy(&data, &m3);
  }

  matrix3f()
  {
    ::matrix3f_set_identity(&data);
  }

  matrix3f(const vector3f& axis, float alpha_degrees)
  {
    ::matrix3f_set_axisangle(&data, &axis.data, alpha_degrees);
  }

  static
  matrix3f 
  axisangle(const vector3f& axis, float alpha_degrees)
  {
    return matrix3f(axis, alpha_degrees);
  }

  //////////////////////////////////////////////////////////////////////////////
  float 
  determinant(void) const
  {
    return ::determinant_m3f(&data);
  }

  void 
  to_axisangle(vector3f& axis, float& angle) const
  {
    ::to_axisangle_m3f(&data, &axis.data, &angle);
  }

  //////////////////////////////////////////////////////////////////////////////
  matrix3f 
  operator *(const matrix3f& mtx) const
  {
    return ::mult_m3f(&data, &mtx.data);
  }

  matrix3f 
  operator *(float scale) const
  {
    return ::mult_m3f_f(&data, scale);
  }

  vector3f 
  operator *(const vector3f& mtx) const
  {
    return ::mult_m3f_vec3f(&data, &mtx.data);
  }

  matrix3f 
  operator +(const matrix3f& mtx) const
  {
    return ::add_m3f(&data, &mtx.data);
  }

  constexpr float& 
  operator[](::M3_RC_XX index)
  {
    return data.data[static_cast<int32_t>(index)];
  }

  constexpr const float& 
  operator[](::M3_RC_XX index) const
  {
    return data.data[static_cast<int32_t>(index)];
  }

  ::matrix3f data;
};

inline
matrix3f 
operator *(float scale, const matrix3f& mtx)
{
  return ::mult_m3f_f(&mtx.data, scale);
}

}

#endif