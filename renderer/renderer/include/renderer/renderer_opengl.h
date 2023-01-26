/**
 * @file renderer_opengl.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef RENDERER_OPENGL_H
#define RENDERER_OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl_data.h>
#include <renderer/internal/module.h>


RENDERER_API
void 
renderer_initialize();

RENDERER_API
void 
renderer_cleanup();

RENDERER_API
void
clear_color_and_depth_buffers();

RENDERER_API
void
flush_operations();

RENDERER_API
void 
update_viewport(const pipeline_t* pipeline);

RENDERER_API
void 
update_projection(const pipeline_t* pipeline);

RENDERER_API
void 
draw_grid(
  pipeline_t* pipeline, 
  float width,
  int32_t lines_per_axis);

RENDERER_API
void 
draw_lines(
  const float* vertices,
  uint32_t vertices_count,
  const uint32_t* indices,
  uint32_t indices_count,
  color_t color,
  float width,
  pipeline_t* pipeline);

RENDERER_API
void 
draw_unit_quads(
  const unit_quad_t* uvs,
  uint32_t uvs_count,
  int32_t texture_id,
  pipeline_t* pipeline);

RENDERER_API
void 
draw_meshes(
  const mesh_render_data_t* mesh,
  const uint32_t* texture_data,
  uint32_t mesh_count,
  pipeline_t* pipeline);

/// @brief bookkeeping is left to the user code. 
RENDERER_API
uint32_t 
upload_to_gpu(
  const char* path,
  const uint8_t* buffer,
  uint32_t width,
  uint32_t height,
  image_format_t format);

RENDERER_API
uint32_t
evict_from_gpu(uint32_t texture_id);


#ifdef __cplusplus
}
#endif

#endif