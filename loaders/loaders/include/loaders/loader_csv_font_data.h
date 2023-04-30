/**
 * @file font_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <stdint.h>

#define GLYPH_COUNT 256


typedef
struct {
  uint32_t x, y;
  uint32_t width, offset;
} glyph_data_t;

typedef
struct {
  float data[6];
} bounds_t;

typedef
struct {
  uint32_t image_width, image_height;
  uint32_t cell_width, cell_height;
  uint32_t font_height, font_width;
  uint32_t start_char;
  glyph_data_t glyphs[GLYPH_COUNT];
  bounds_t bounds[GLYPH_COUNT];
} loader_csv_font_data_t;

#endif