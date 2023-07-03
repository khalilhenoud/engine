/**
 * @file renderer_opengl_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef RENDERER_OPENGL_DATA_H
#define RENDERER_OPENGL_DATA_H

#include <stdint.h>


typedef
struct unit_quad_t {
  float data[6];
} unit_quad_t;

typedef
struct color_t {
  float data[4];
} color_t;

typedef
enum image_format_t {
  RENDERER_OPENGL_RGBA,
  RENDERER_OPENGL_BGRA,
  RENDERER_OPENGL_RGB,
  RENDERER_OPENGL_BGR,
  RENDERER_OPENGL_LA,           /// Luminance/Alpha.
  RENDERER_OPENGL_L,
  RENDERER_OPENGL_A,
  RENDERER_OPENGL_IMAGE_FORMAT_COUNT
} image_format_t;

typedef
struct mesh_render_data_t {
  const float* vertices;    // 3 floats per vertex.
  const float* normals;     // 3 floats per vertex.
  const float* uv_coords;   // 3 floats per vertex.
  uint32_t vertex_count;    // applies to the previous 3 arrays.
  const uint32_t* indices;
  uint32_t indices_count;
  color_t ambient;
  color_t diffuse;
  color_t specular;
} mesh_render_data_t;

#endif