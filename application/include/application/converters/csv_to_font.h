/**
 * @file csv_to_font.h
 * @author khalilhenoud@gmail.com
 * @brief acts like a glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CSV_TO_FONT
#define CSV_TO_FONT

#include <memory>
#include <string>


typedef struct allocator_t allocator_t;

namespace entity {
  struct font;
}

std::unique_ptr<entity::font>
load_font(
  std::string& data_set,
  std::string& image_file, 
  std::string& data_file, 
  const allocator_t* allocator);

#endif