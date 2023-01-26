/**
 * @file loader_csv.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <loaders/utils.h>
#include <loaders/loader_csv.h>


typedef
struct mem_token_t {
  uint32_t* ptr;
  const char* str;
} mem_token_t;

loader_csv_font_data_t*
load_csv(
  const char* path, 
  const allocator_t* allocator)
{
  loader_csv_font_data_t* font_data = (loader_csv_font_data_t*)allocator->mem_alloc(sizeof(loader_csv_font_data_t));
  size_t file_size;
  const char* data = read_file_as_ascii(path, allocator, &file_size);
  mem_token_t* ptr_token = (mem_token_t*)allocator->mem_cont_alloc(2048, sizeof(mem_token_t));
  uint32_t used = 0;

  ptr_token[used].ptr = (uint32_t*)&(font_data->image_width);
  ptr_token[used++].str = "Image Width";
  ptr_token[used].ptr = (uint32_t*)&(font_data->image_height); 
  ptr_token[used++].str = "Image Height";
  ptr_token[used].ptr = (uint32_t*)&(font_data->cell_width);
  ptr_token[used++].str = "Cell Width";
  ptr_token[used].ptr = (uint32_t*)&(font_data->cell_height); 
  ptr_token[used++].str = "Cell Height";
  ptr_token[used].ptr = (uint32_t*)&(font_data->start_char);
  ptr_token[used++].str = "Start Char";
  ptr_token[used].ptr = (uint32_t*)&(font_data->font_height);
  ptr_token[used++].str = "Font Height";
  ptr_token[used].ptr = (uint32_t*)&(font_data->font_width);
  ptr_token[used++].str = "Font Width";

  for (uint32_t i = 0; i < GLYPH_COUNT; ++i) {
    ptr_token[used].ptr = (uint32_t*)&(font_data->glyphs[i].width);
    ptr_token[used++].str = "Base Width";
  }
                                                  
  for (uint32_t i = 0; i < GLYPH_COUNT; ++i) {
    ptr_token[used].ptr = (uint32_t*)&(font_data->glyphs[i].offset);
    ptr_token[used++].str = "Width Offset";
  }
                                                  
  for (uint32_t i = 0; i < GLYPH_COUNT; ++i) {
    ptr_token[used].ptr = (uint32_t*)&(font_data->glyphs[i].x);
    ptr_token[used++].str = "X Offset";
  }

  for (uint32_t i = 0; i < GLYPH_COUNT; ++i) {
    ptr_token[used].ptr = (uint32_t*)&(font_data->glyphs[i].y);
    ptr_token[used++].str = "Y Offset";
  }

  {
    // glyph format: 'Char 102 Width Offset,0'
    const char* start = data;
    for (uint32_t i = 0; i < used; ++i) {
      start = strstr(start, ptr_token[i].str);
      start += strlen(ptr_token[i].str);
      start = strchr(start, ',');
      sscanf(start, ",%u", ptr_token[i].ptr);
    }
  }

  {
    uint32_t column = font_data->image_width / font_data->cell_width;
    uint32_t row = font_data->image_height / font_data->cell_height;
    uint32_t glyph_count = column * row;
    assert(
      '$' >= font_data->start_char && 
      '$' < (font_data->start_char + glyph_count));

    {
      // TODO: define your own system for how this is represented, irrespective
      // of the rendering api.
      float dx = 1.f / (float)column;
      float dy = 1.f / (float)row;
      float dheight = (float)font_data->font_height / font_data->cell_height;
      for (uint32_t i = 0; i < glyph_count; ++i) {
        uint32_t c = i + font_data->start_char;
        uint32_t c_row = i / column;
        uint32_t c_column = i - c_row * column;
        float dwidth = (float)(font_data->glyphs[c].width) / font_data->cell_width;
        font_data->bounds[c].data[0] = dx * (float)c_column;
        font_data->bounds[c].data[1] = 1 - dy * (float)c_row;
        font_data->bounds[c].data[2] = font_data->bounds[c].data[0] + dx * dwidth;
        font_data->bounds[c].data[3] = font_data->bounds[c].data[1] - dy * dheight;
        font_data->bounds[c].data[4] = dwidth;
        font_data->bounds[c].data[5] = dheight;
      }
    }
  }

  allocator->mem_free(ptr_token);
  allocator->mem_free((void*)data);
  return font_data;
}

void
free_csv(
  loader_csv_font_data_t* data, 
  const allocator_t* allocator)
{
  allocator->mem_free(data);
}