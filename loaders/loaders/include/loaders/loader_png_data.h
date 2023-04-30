/**
 * @file loader_png_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef LOADER_PNG_DATA_H
#define LOADER_PNG_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef
enum {
  LOADER_PNG_RGBA,
  LOADER_PNG_BGRA,
  LOADER_PNG_RGB,
  LOADER_PNG_BGR,
  LOADER_PNG_LA,       // Luminance/Alpha.
  LOADER_PNG_L,
  LOADER_PNG_A
} loader_image_format_t;

typedef
struct {
  uint32_t width;
  uint32_t height;
  loader_image_format_t format;
  uint8_t* buffer;
  uint32_t total_buffer_size;
} loader_png_data_t;


#ifdef __cplusplus
}
#endif

#endif