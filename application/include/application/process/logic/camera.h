/**
 * @file camera.h
 * @author khalilhenoud@gmail.com
 * @brief first attempt at decompose logic into separate files.
 * @version 0.1
 * @date 2023-10-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CAMERA_PROCESS_LOGIC_H
#define CAMERA_PROCESS_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct bvh_t bvh_t;
typedef struct capsule_t capsule_t;
typedef struct camera_t camera_t;
typedef struct pipeline_t pipeline_t;

void
camera_update(
  camera_t* camera, bvh_t* bvh, capsule_t* capsule, pipeline_t* pipeline);

void
recenter_camera_cursor(void);

#ifdef __cplusplus
}
#endif

#endif