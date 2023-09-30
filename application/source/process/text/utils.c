/**
 * @file utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-09-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <application/process/text/utils.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>
#include <entity/c/runtime/font.h>


void
render_text_to_screen(
  font_runtime_t* font, 
  uint32_t font_image_id,
  pipeline_t* pipeline, 
  const char** text, 
  uint32_t count)
{
  if (!(font && pipeline && text))
    return;

  {
    float left, right, bottom, top, nearz, farz;
    float viewport_left, viewport_right, viewport_bottom, viewport_top;
    glyph_bounds_t from;
    unit_quad_t bounds[512];
    uint32_t str_length = 0;

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
        0, 
        viewport_bottom - ((i + 1) * (float)font->font_height), -2);
      pre_scale(
        pipeline, 
        (float)font->cell_width, 
        (float)font->cell_height, 0);
      draw_unit_quads(bounds, str_length, font_image_id, pipeline);
      pop_matrix(pipeline);

      set_perspective(pipeline, left, right, bottom, top, nearz, farz);
      update_projection(pipeline);
    }
  }
}