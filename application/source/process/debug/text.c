/**
 * @file text.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <assert.h>
#include <string.h>
#include <application/process/debug/text.h>
#include <application/process/text/utils.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>
#include <entity/c/runtime/font.h>

#define DEBUG_TEXT_MAX_SIZE         256
#define DEBUG_TEXT_MAX_ALLOWED      128


typedef
struct {
  char text[DEBUG_TEXT_MAX_SIZE];
  debug_color_t color;
  float x, y;
} debug_text_t;

typedef
struct {
  debug_text_t text[DEBUG_TEXT_MAX_ALLOWED];
  uint32_t used;
} debug_text_frame_t;

////////////////////////////////////////////////////////////////////////////////
static debug_text_frame_t debug_frame;

void
add_debug_text_to_frame(
  const char* text,
  debug_color_t color,
  float x,
  float y)
{
  assert(
    strlen(text) < DEBUG_TEXT_MAX_SIZE && 
    "'text' is too long, keep it less than 256!");
  assert(
    debug_frame.used < DEBUG_TEXT_MAX_ALLOWED && 
    "exceeded max allowed debug text entries per frame!");

  {
    debug_text_t *dst = debug_frame.text + debug_frame.used++;
    memset(dst->text, 0, sizeof(dst->text));
    memcpy(dst->text, text, strlen(text));

    dst->color = color;
    dst->x = x;
    dst->y = y;
  }
}

// TODO: this could be optimized, enable batch draw.
void
draw_debug_text_frame(
  pipeline_t *pipeline,
  font_runtime_t *font,
  const uint32_t font_image_id)
{
  const char *text;
  color_t color;
  for (uint32_t i = 0; i < debug_frame.used; ++i) {
    text = debug_frame.text[i].text;
    memcpy(color.data, debug_frame.text[i].color.data, sizeof(color.data));
    render_text_to_screen(
      font, 
      font_image_id, 
      pipeline, 
      &text,
      1, 
      &color, 
      debug_frame.text[i].x, 
      debug_frame.text[i].y);
  }

  debug_frame.used = 0;
}