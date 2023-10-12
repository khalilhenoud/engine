/**
 * @file to_render_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-09-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SCENE_RENDER_DATA
#define SCENE_RENDER_DATA

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <library/string/fixed_string.h>
#include <entity/c/mesh/color.h>


typedef struct scene_t scene_t; 
typedef struct mesh_t mesh_t;
typedef struct allocator_t allocator_t;
typedef struct mesh_render_data_t mesh_render_data_t;
typedef struct font_runtime_t font_runtime_t;
typedef struct camera_t camera_t;
typedef struct texture_runtime_t texture_runtime_t;

// TODO(khalil): support matrix hierarchy when rendering.
typedef
struct packaged_mesh_data_t {
  uint32_t count;
  mesh_render_data_t* mesh_render_data;
  texture_runtime_t* texture_runtimes;
  uint32_t* texture_ids;
} packaged_mesh_data_t;

typedef
struct packaged_font_data_t {
  uint32_t count;
  font_runtime_t* fonts;
  texture_runtime_t* texture_runtimes;
  uint32_t* texture_ids;
} packaged_font_data_t;

typedef
struct packaged_camera_data_t {
  camera_t* cameras;
  uint32_t count;
} packaged_camera_data_t;

typedef
struct packaged_scene_render_data_t {
  packaged_mesh_data_t mesh_data;
  packaged_font_data_t font_data;
  packaged_camera_data_t camera_data;
} packaged_scene_render_data_t;

void
free_render_data(
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator);

void
free_mesh_render_data(
  packaged_mesh_data_t* mesh_data, 
  const allocator_t* allocator);

packaged_scene_render_data_t* 
load_scene_render_data(
  scene_t* scene, 
  const allocator_t* allocator);

packaged_mesh_data_t*
load_mesh_renderer_data(
  mesh_t* mesh, 
  const color_rgba_t color, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif