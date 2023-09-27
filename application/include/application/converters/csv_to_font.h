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

#ifdef __cplusplus
extern "C" {
#endif


typedef struct font_t font_t;
typedef struct allocator_t allocator_t;

// data_set is the base directory.
// call free_font() to delete the font.
font_t*
load_font(
  const char* data_set,
  const char* image_file, 
  const char* data_file, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif