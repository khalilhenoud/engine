/**
 * @file camera.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CAMERA_H_INCLUDE
#define CAMERA_H_INCLUDE

#include <math/matrix4f.h>


namespace entity {

struct 
camera {

  camera()
    : m_position(0.f, 0.f, 0.f)
    , m_lookatdirection(0.f, 0.f, -1.f)
    , m_upvector(0.f, 1.f, 0.f) 
  {}

  void 
  look_at(
    const math::vector3f& position, 
    const math::vector3f& lookat, 
    const math::vector3f& upvector)
  {
    m_lookatdirection = lookat - position;
    m_lookatdirection.normalize();
    m_upvector = upvector;
    m_upvector.normalize();
    m_position = position;
  }

  math::matrix4f 
  get_view_transformation() const
  {
    math::matrix4f translation = math::matrix4f::translation(
      -m_position.x, 
      -m_position.y, 
      -m_position.z);
    math::matrix4f crossproduct = math::matrix4f::cross_product(m_upvector);
    math::vector3f tmp = -m_lookatdirection;
    math::vector3f result = crossproduct.mult_vec(tmp);
    math::matrix4f camerarotation;
    camerarotation[M4_RC_00] = result.x;
    camerarotation[M4_RC_01] = result.y;
    camerarotation[M4_RC_02] = result.z;
    camerarotation[M4_RC_10] = m_upvector.x;
    camerarotation[M4_RC_11] = m_upvector.y;
    camerarotation[M4_RC_12] = m_upvector.z;
    camerarotation[M4_RC_20] = -m_lookatdirection.x;
    camerarotation[M4_RC_21] = -m_lookatdirection.y;
    camerarotation[M4_RC_22] = -m_lookatdirection.z;
    return camerarotation * translation;
  }
  
  math::vector3f m_position;
  math::vector3f m_lookatdirection;
  math::vector3f m_upvector;
};

}

#endif
