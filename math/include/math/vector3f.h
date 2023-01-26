/**
 * @file vector3f.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CPP_VECTOR3F_H
#define CPP_VECTOR3F_H

#include <math/c/vector3f.h>


namespace math {

struct 
vector3f {
  vector3f(const ::vector3f& v3)
  {
    ::vector3f_copy(&data, &v3);
  }

  vector3f()
  {
    ::vector3f_set_1f(&data, 0.f);
  }

  vector3f(float value)
  {
    ::vector3f_set_1f(&data, value);
  }
  
  vector3f(const vector3f& other)
  {
    ::vector3f_copy(&data, &other.data);
  }

  vector3f(float xvalue, float yvalue, float zvalue)
  {
    ::vector3f_set_3f(&data, xvalue, yvalue, zvalue);
  }

  vector3f(const float* f)
  {
    ::vector3f_set_a3f(&data, f);
  }

  vector3f(const vector3f& a, const vector3f& b)
  {
    ::vector3f_set_diff_v3f(&data, &a.data, &b.data);
  }

  vector3f& 
  operator =(const vector3f& other)
  {
    ::vector3f_copy(&data, &other.data);
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  float 
  length() const
  {
    return ::length_v3f(&data);
  }

  float 
  length_square() const
  {
    return ::length_squared_v3f(&data);
  }

  float 
  dot_product(const vector3f& vec) const
  {
    return ::dot_product_v3f(&data, &vec.data);
  }

  vector3f 
  cross_product(const vector3f& vec) const
  {
    return ::cross_product_v3f(&data, &vec.data);
  }

  void 
  normalize()
  {
    ::normalize_set_v3f(&data);
  }

  //////////////////////////////////////////////////////////////////////////////
  bool 
  operator ==(const vector3f& vec) const
  {
    return ::equal_to_v3f(&data, &vec.data) != 0;
  }

  vector3f 
  operator -() const
  {
    return ::negate_v3f(&data);
  }

  vector3f 
  operator -(const vector3f& other) const
  {
    return vector3f(*this, other);
  }

  vector3f& 
  operator -=(const vector3f& rhs)
  {
    ::diff_set_v3f(&data, &rhs.data);
    return *this;
  }

  vector3f 
  operator +(const vector3f& rhs) const
  {
    return add_v3f(&data, &rhs.data);
  }

  vector3f& 
  operator +=(const vector3f& rhs)
  {
    ::add_set_v3f(&data, &rhs.data);
    return *this;
  }

  vector3f 
  operator *(float scale) const
  {
    return ::mult_v3f(&data, scale);
  }

  vector3f& 
  operator *=(float scale)
  {
    ::mult_set_v3f(&data, scale);
    return *this;
  }

  vector3f 
  operator /(float scale) const
  {
    return ::mult_v3f(&data, 1.f/scale);
  }

  vector3f& 
  operator /=(float scale)
  {
    ::mult_set_v3f(&data, 1.f/scale);
    return *this;
  }

  constexpr float& 
  operator[](int32_t index)
  {
    if (index == 0)
      return x;
    if (index == 1)
      return y;
    if (index == 2)
      return z;
    return x;
  }

  ::vector3f data;
  float& x = data.data[0], &y = data.data[1], &z = data.data[2];
};

inline 
vector3f 
operator *(float scale, const vector3f& vec)
{
  return ::mult_v3f(&vec.data, scale);
}

}

#endif