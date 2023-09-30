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

#ifdef __cplusplus
extern "C" {
#endif


typedef struct texture_runtime_t texture_runtime_t;
typedef texture_runtime_t image_t;
typedef struct allocator_t allocator_t;

void
load_image_buffer(
  const char* data_set, 
  image_t* image, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif