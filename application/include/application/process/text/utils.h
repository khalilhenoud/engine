/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief text helpers 
 * @version 0.1
 * @date 2023-09-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef HELPERS_TEXT_UTILS_H
#define HELPERS_TEXT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef struct font_runtime_t font_runtime_t;
typedef struct pipeline_t pipeline_t;

void
render_text_to_screen(
  font_runtime_t* font, 
  uint32_t font_image_id,
  pipeline_t* pipeline, 
  const char** text, 
  uint32_t count);

#ifdef __cplusplus
}
#endif

#endif