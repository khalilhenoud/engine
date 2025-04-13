/**
 * @file camera.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-04-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef CAMERA_PROCESS_LOGIC_H
#define CAMERA_PROCESS_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math/c/vector3f.h>


typedef struct camera_t camera_t;

void
camera_init(
  camera_t* camera,
  point3f position,
  vector3f look_at,
  vector3f up);

void
camera_update(
  camera_t* camera, 
  float delta_time);

#ifdef __cplusplus
}
#endif

#endif