/**
 * @file image.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <string>
#include <vector>
#include <cassert>


namespace entity {

struct 
image {

  //////////////////////////////////////////////////////////////////////////////
  enum class format : uint32_t {
    RGBA,
    BGRA,
    RGB,
    BGR,
    LA,             /// Luminance/Alpha.
    L,
    A
  };

  //////////////////////////////////////////////////////////////////////////////
  image(const std::string& filepath)
    : m_path{ filepath }
  {}

  uint32_t
  get_width() const 
  { 
    return m_width; 
  }

  uint32_t
  get_height() const 
  { 
    return m_height; 
  }

  format
  get_format() const 
  { 
    return m_format; 
  }

  uint32_t
  get_component_number() const
  {
    switch (m_format)
    {
    case format::RGBA:
    case format::BGRA:
      return 4;
      break;
    case format::RGB:
    case format::BGR:
      return 3;
      break;
    case format::LA:
      return 2;
      break;
    case format::L:
    case format::A:
      return 1;
      break;
    }

    assert(false);
    return 0;
  }

  bool
  is_component_power_2() const
  {
    auto components = get_component_number();
    return (components & (components - 1)) == 0;
  }

  std::string 
  get_extension() const 
  { 
    return m_path.substr(m_path.find_last_of('.'), m_path.length()); 
  }

  bool 
  is_loaded() const 
  { 
    return !m_buffer.empty(); 
  }

  void
  clear() 
  { 
    m_buffer.clear(); 
  }

  const uint8_t* 
  get_buffer()
  {
    assert(is_loaded());
    assert(m_buffer.size() != 0);
    return &m_buffer[0];
  }

  const std::string m_path;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  format m_format = format::RGBA;
  std::vector<uint8_t> m_buffer;
};

}

#endif