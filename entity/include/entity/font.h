/**
 * @file font.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef FONT_H_INCLUDE
#define FONT_H_INCLUDE

#include <string>
#include <array>


namespace entity {

/// NOTE: This has to guarantee including $ sign char.
struct 
font {

  //////////////////////////////////////////////////////////////////////////////
  using bounds = std::array<float, 6>;

  struct glyph_info {
    uint32_t m_x = 0, m_y = 0;
    uint32_t m_width = 0;
    uint32_t m_widthoffset = 0;
  };

  //////////////////////////////////////////////////////////////////////////////
  font(std::string& imagefile, std::string& datafile)
    : m_imagefile(imagefile)
    , m_datafile(datafile)
  {}

  uint32_t
  get_glyph_count() const
  {
    return (m_imagewidth / m_cellwidth) * (m_imageheight / m_cellheight);
  }

  uint32_t
  get_column_count() const
  {
    return m_imagewidth / m_cellwidth;
  }

  uint32_t
  get_row_count() const
  {
    return m_imageheight / m_cellheight;
  }

  uint32_t
  get_image_width() const
  {
    return m_imagewidth;
  }

  uint32_t
  get_image_height() const
  {
    return m_imageheight;
  }

  uint32_t
  get_cell_width() const
  {
    return m_cellwidth;
  }

  uint32_t
  get_cell_height() const
  {
    return m_cellheight;
  }

  uint32_t
  get_font_height() const
  {
    return m_fontheight;
  }

  bounds
  get_uv_bounds(char c)
  {
    c = has_char(c) ? c : '$';
    return m_bounds[c];
  }
  
  bool
  has_char(char c)
  {
    uint32_t total = get_glyph_count();
    auto cui = (uint32_t)c;
    return cui >= m_startchar && cui < (m_startchar + total);
  }

  //////////////////////////////////////////////////////////////////////////////
  static constexpr uint32_t s_gliphcount = 256;

  const std::string m_imagefile;
  const std::string m_datafile;

  uint32_t m_imagewidth = 0, m_imageheight = 0;
  uint32_t m_cellwidth = 0, m_cellheight = 0;
  uint32_t m_fontwidth = 0, m_fontheight = 0;
  uint32_t m_startchar = 0;

  glyph_info m_glyphs[s_gliphcount];
  bounds m_bounds[s_gliphcount];
};

}

#endif