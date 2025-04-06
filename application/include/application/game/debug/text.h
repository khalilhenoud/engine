/**
 * @file text.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef DEBUG_TEXT_H
#define DEBUG_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <application/game/debug/color.h>


typedef struct pipeline_t pipeline_t;
typedef struct font_runtime_t font_runtime_t;

void
add_debug_text_to_frame(
  const char* text,
  debug_color_t color,
  float x,
  float y);

void
draw_debug_text_frame(
  pipeline_t *pipeline,
  font_runtime_t *font,
  const uint32_t font_image_id);

void
render_text_to_screen(
  font_runtime_t* font, 
  uint32_t font_image_id,
  pipeline_t* pipeline, 
  const char** text, 
  uint32_t count,
  const debug_color_t color,
  float x,
  float y);

#ifdef __cplusplus
}
#endif

#endif