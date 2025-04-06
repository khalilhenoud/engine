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
  for (uint32_t i = 0; i < debug_frame.used; ++i) {
    text = debug_frame.text[i].text;
    render_text_to_screen(
      font, 
      font_image_id, 
      pipeline, 
      &text,
      1, 
      debug_frame.text[i].color, 
      debug_frame.text[i].x, 
      debug_frame.text[i].y);
  }

  debug_frame.used = 0;
}

void
render_text_to_screen(
  font_runtime_t* font, 
  uint32_t font_image_id,
  pipeline_t* pipeline, 
  const char** text, 
  uint32_t count,
  const debug_color_t color,
  float x,
  float y)
{
  if (!(font && pipeline && text))
    return;

  {
    float left, right, bottom, top, nearz, farz;
    float viewport_left, viewport_right, viewport_bottom, viewport_top;
    color_t r_color;
    glyph_bounds_t from;
    unit_quad_t bounds[512];
    uint32_t str_length = 0;

    memcpy(r_color.data, color.data, sizeof(color.data));

    for (uint32_t i = 0; i < count; ++i) {
      const char* str = text[i];
      str_length = strlen(str);
      for (uint32_t k = 0; k < str_length; ++k) {
        char c = str[k];
        get_glyph_bounds(font, c, &from);
        bounds[k].data[0] = from[0];
        bounds[k].data[1] = from[1];
        bounds[k].data[2] = from[2];
        bounds[k].data[3] = from[3];
        bounds[k].data[4] = from[4];
        bounds[k].data[5] = from[5];
      }

      get_frustum(pipeline, &left, &right, &bottom, &top, &nearz, &farz);
      get_viewport_info(
        pipeline, 
        &viewport_left, 
        &viewport_top, 
        &viewport_right, 
        &viewport_bottom);
      set_orthographic(
        pipeline, 
        viewport_left, 
        viewport_right, 
        viewport_top, 
        viewport_bottom, 
        nearz, 
        farz);
      update_projection(pipeline);

      push_matrix(pipeline);
      load_identity(pipeline);
      pre_translate(
        pipeline, 
        x, 
        viewport_bottom - ((i + 1) * (float)font->font_height) - y, -2);
      pre_scale(
        pipeline, 
        (float)font->cell_width, 
        (float)font->cell_height, 0);
      draw_unit_quads(bounds, str_length, font_image_id, r_color, pipeline);
      pop_matrix(pipeline);

      set_perspective(pipeline, left, right, bottom, top, nearz, farz);
      update_projection(pipeline);
    }
  }
}