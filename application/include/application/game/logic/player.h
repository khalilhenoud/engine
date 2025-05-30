/**
  *@file player.h
  *@author khalilhenoud@gmail.com
  *@brief first attempt at decompose logic into separate files.
  *@version 0.1
  *@date 2023-10-04
  *
  *@copyright Copyright (c) 2023
  *
 */
#ifndef PLAYER_PROCESS_LOGIC_H
#define PLAYER_PROCESS_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math/c/vector3f.h>


typedef struct bvh_t bvh_t;
typedef struct camera_t camera_t;

void
player_init(
  point3f player_start,
  float player_angle,
  camera_t *camera,
  bvh_t *bvh);

void
player_update(float delta_time);

#ifdef __cplusplus
}
#endif

#endif