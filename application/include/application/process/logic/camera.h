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


typedef struct camera_t camera_t;

void
camera_update(camera_t* camera);

void
recenter_camera_cursor(void);

#ifdef __cplusplus
}
#endif

#endif