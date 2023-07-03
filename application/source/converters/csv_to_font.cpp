/**
 * @file csv_to_font.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <application/converters/csv_to_font.h>
#include <loaders/loader_csv.h>
#include <entity/font.h>


std::unique_ptr<entity::font>
load_font(
  std::string& data_set,
  std::string& image_file, 
  std::string& data_file, 
  const allocator_t* allocator)
{
  auto file = data_set + data_file;
  loader_csv_font_data_t* data_font = load_csv(file.c_str(), allocator);
  std::unique_ptr<entity::font> font = 
    std::make_unique<entity::font>(image_file, data_file);
  font->m_imagewidth  = data_font->image_width;
  font->m_imageheight = data_font->image_height;
  font->m_cellwidth   = data_font->cell_width;
  font->m_cellheight  = data_font->cell_height;
  font->m_fontheight  = data_font->font_height;
  font->m_fontwidth   = data_font->font_width;
  font->m_startchar   = data_font->start_char;

  for (uint32_t i = 0; i < entity::font::s_gliphcount; ++i) {
    font->m_glyphs[i] = { 
      data_font->glyphs[i].x, 
      data_font->glyphs[i].y,
      data_font->glyphs[i].width,
      data_font->glyphs[i].offset};

    font->m_bounds[i][0] = data_font->bounds[i].data[0];
    font->m_bounds[i][1] = data_font->bounds[i].data[1];
    font->m_bounds[i][2] = data_font->bounds[i].data[2];
    font->m_bounds[i][3] = data_font->bounds[i].data[3];
    font->m_bounds[i][4] = data_font->bounds[i].data[4];
    font->m_bounds[i][5] = data_font->bounds[i].data[5];
  }

  free_csv(data_font, allocator);
  return font;
}