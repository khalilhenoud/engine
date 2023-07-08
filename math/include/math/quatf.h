/**
 * @file quatf.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CPP_QUATF_H
#define CPP_QUATF_H

#include <cmath>
#include <math/common.h>
#include <math/vector3f.h>
#include <math/matrix3f.h>
#include <math/matrix4f.h>
#include <math/c/quatf.h>


namespace math {

struct 
quatf {
  quatf(float s, float x, float y, float z)
  {
    ::quatf_set_4f(&data, s, x, y, z);
  }

  quatf()
  {
    ::quatf_set_identity(&data);
  }

  quatf(float* fill)
  {
    ::quatf_set_a4f(&data, fill)
  }

  quatf(const vector3f& axis, float angle_radian)
  {
    ::quatf_set_from_axis_angle(&data, &axis.data, angle_radian);
  }

  quatf(const matrix3f& rotation)
  {
    ::quatf_set_from_rotation_matrix3f(&data, &rotation.data);
  }

  quatf(const matrix4f& rotation)
  {
    ::quatf_set_from_rotation_matrix4f(&data, &rotation.data);
  }

  quatf(const ::quatf& quat)
  {
    ::quatf_copy(&data, &quat);
  }

  static
  quatf
  axisangle(const vector3f& axis, float angle_radian)
  {
    return quatf(axis, angle_radian);
  }

  static
  quatf
  from_rotation_matrix(const matrix3f& rotation)
  {
    return quatf(rotation);
  }

  static
  quatf
  from_rotation_matrix(const matrix4f& rotation)
  {
    return quatf(rotation);
  }

  //////////////////////////////////////////////////////////////////////////////
  void
  to_axisangle(vector3f& axis, float& angle_radian) const
  {
    ::get_quatf_axis_angle(&data, &axis.data, &angle_radian);
  }
  
  //////////////////////////////////////////////////////////////////////////////
  float
  length(void) const
  {
    return ::length_quatf(&data);
  }

  float
  length_squared(void) const
  {
    return ::length_squared_quatf(&data);
  }

  float
  dot_product(const quatf& quat) const
  {
    return ::dot_product_quatf(&data, &quat.data);
  }

  //////////////////////////////////////////////////////////////////////////////
  quatf
  inverse(void) const
  {
    return ::inverse_quatf(&data);
  }

  void
  set_inverse(void)
  {
    ::inverse_set_quatf(&data);
  }

  quatf
  conjugate(void) const
  {
    return ::conjugate_quatf(&data);
  }

  void
  set_conjugate(void)
  {
    ::conjugate_set_quatf(&data);
  }

  //////////////////////////////////////////////////////////////////////////////
  quatf
  operator *(float scale) const
  {
    return ::mult_quatf_f(&data, scale);
  }

  quatf
  operator +(const quatf& rhs) const
  {
    return ::add_quatf(&data, &rhs.data);
  }

  quatf
  operator *(const quatf& rhs) const
  {
    return ::mult_quatf(&data, &rhs.data);
  }

  vector3f
  operator *(const vector3f& vec3) const
  {
    return ::mult_quatf_v3f(&data, &vec3.data);
  }

  constexpr float& 
  operator[](QUAT_DATA index)
  {
    return data.data[static_cast<int32_t>(index)];
  }

  constexpr const float& 
  operator[](QUAT_DATA index) const
  {
    return data.data[static_cast<int32_t>(index)];
  }

  ::quatf data;
};

inline
quatf 
operator *(float scale, const quatf& quat)
{
  return ::mult_quatf_f(&quat.data, scale);
}

}

#endif