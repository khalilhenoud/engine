/**
 * @file csv_to_font.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <application/game/rendering/load_font.h>
#include <library/string/fixed_string.h>
#include <library/string/cstring.h>
#include <library/allocator/allocator.h>
#include <loaders/loader_csv.h>
#include <entity/c/misc/font.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>


void
load_font_inplace(
  const char* data_set, 
  const font_t* font, 
  font_runtime_t* runtime, 
  const allocator_t* allocator)
{
  
  {
    fixed_str_t csv_file;
    memset(csv_file.data, 0, sizeof(csv_file.data));
    sprintf(csv_file.data, "%s%s", data_set, font->data_file->str);
    loader_csv_font_data_t* data_font = load_csv(csv_file.data, allocator);
    
    runtime->image_width  = data_font->image_width;
    runtime->image_height = data_font->image_height;
    runtime->cell_width   = data_font->cell_width;
    runtime->cell_height  = data_font->cell_height;
    runtime->font_width   = data_font->font_width;
    runtime->font_height  = data_font->font_height;
    runtime->start_char   = data_font->start_char;

    for (uint32_t i = 0; i < FONT_GLYPH_COUNT; ++i) {
      runtime->glyphs[i].x = data_font->glyphs[i].x; 
      runtime->glyphs[i].y = data_font->glyphs[i].y; 
      runtime->glyphs[i].width = data_font->glyphs[i].width; 
      runtime->glyphs[i].width_offset = data_font->glyphs[i].offset; 

      runtime->bounds[i][0] = data_font->bounds[i].data[0];
      runtime->bounds[i][1] = data_font->bounds[i].data[1];
      runtime->bounds[i][2] = data_font->bounds[i].data[2];
      runtime->bounds[i][3] = data_font->bounds[i].data[3];
      runtime->bounds[i][4] = data_font->bounds[i].data[4];
      runtime->bounds[i][5] = data_font->bounds[i].data[5];
    }

    free_csv(data_font, allocator);
  }
}

font_runtime_t*
load_font(
  const char* data_set,
  const char* image_file, 
  const char* data_file, 
  const allocator_t* allocator)
{
  font_t* font = font_create(image_file, data_file, allocator);
  font_runtime_t* runtime = create_font_runtime(font, allocator);
  font_free(font, allocator);

  {
    fixed_str_t csv_file;
    memset(csv_file.data, 0, sizeof(csv_file.data));
    sprintf(csv_file.data, "%s%s", data_set, data_file);
    loader_csv_font_data_t* data_font = load_csv(csv_file.data, allocator);
    
    runtime->image_width  = data_font->image_width;
    runtime->image_height = data_font->image_height;
    runtime->cell_width   = data_font->cell_width;
    runtime->cell_height  = data_font->cell_height;
    runtime->font_width   = data_font->font_width;
    runtime->font_height  = data_font->font_height;
    runtime->start_char   = data_font->start_char;

    for (uint32_t i = 0; i < FONT_GLYPH_COUNT; ++i) {
      runtime->glyphs[i].x = data_font->glyphs[i].x; 
      runtime->glyphs[i].y = data_font->glyphs[i].y; 
      runtime->glyphs[i].width = data_font->glyphs[i].width; 
      runtime->glyphs[i].width_offset = data_font->glyphs[i].offset; 

      runtime->bounds[i][0] = data_font->bounds[i].data[0];
      runtime->bounds[i][1] = data_font->bounds[i].data[1];
      runtime->bounds[i][2] = data_font->bounds[i].data[2];
      runtime->bounds[i][3] = data_font->bounds[i].data[3];
      runtime->bounds[i][4] = data_font->bounds[i].data[4];
      runtime->bounds[i][5] = data_font->bounds[i].data[5];
    }

    free_csv(data_font, allocator);
  }
 
  return runtime;
}