/**
 * @file face.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef DEBUG_FACE_H
#define DEBUG_FACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <application/process/debug/color.h>


typedef struct pipeline_t pipeline_t;
typedef struct face_t face_t;
typedef struct vector3f vector3f;

void
add_debug_face_to_frame(
  face_t *face, 
  vector3f *normal,
  debug_color_t color,
  int32_t thickness);

void
draw_debug_face_frame(
  pipeline_t *pipeline,
  const int32_t disable_depth);

#ifdef __cplusplus
}
#endif

#endif