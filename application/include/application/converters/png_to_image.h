/**
 * @file png_to_image.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef PNG_TO_IMAGE
#define PNG_TO_IMAGE

#include <string>


typedef struct allocator_t allocator_t;

namespace entity {
  struct image;
}

bool
load_image(
  std::string& dataset, 
  entity::image& image, 
  const allocator_t* allocator);

#endif