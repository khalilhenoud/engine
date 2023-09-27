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
#include <library/string/fixed_string.h>
#include <library/allocator/allocator.h>
#include <application/converters/csv_to_font.h>
#include <loaders/loader_csv.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>


font_t*
load_font(
  const char* data_set,
  const char* image_file, 
  const char* data_file, 
  const allocator_t* allocator)
{
  font_t* font = create_font(image_file, data_file, allocator);

  fixed_str_t csv_file;
  memset(csv_file.data, 0, sizeof(csv_file.data));
  sprintf(csv_file.data, "%s%s", data_set, data_file);
  loader_csv_font_data_t* data_font = load_csv(csv_file.data, allocator);
  
  font->image_width  = data_font->image_width;
  font->image_height = data_font->image_height;
  font->cell_width   = data_font->cell_width;
  font->cell_height  = data_font->cell_height;
  font->font_width   = data_font->font_width;
  font->font_height  = data_font->font_height;
  font->start_char   = data_font->start_char;

  for (uint32_t i = 0; i < FONT_GLYPH_COUNT; ++i) {
    font->glyphs[i].x = data_font->glyphs[i].x; 
    font->glyphs[i].y = data_font->glyphs[i].y; 
    font->glyphs[i].width = data_font->glyphs[i].width; 
    font->glyphs[i].width_offset = data_font->glyphs[i].offset; 

    font->bounds[i][0] = data_font->bounds[i].data[0];
    font->bounds[i][1] = data_font->bounds[i].data[1];
    font->bounds[i][2] = data_font->bounds[i].data[2];
    font->bounds[i][3] = data_font->bounds[i].data[3];
    font->bounds[i][4] = data_font->bounds[i].data[4];
    font->bounds[i][5] = data_font->bounds[i].data[5];
  }

  free_csv(data_font, allocator);
  return font;
}