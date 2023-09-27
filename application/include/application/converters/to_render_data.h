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

typedef
struct render_data_t {
  mesh_render_data_t* mesh_render_data;
  uint32_t mesh_count;

  fixed_str_t* textures;
  uint32_t texture_count;

  // to be filled by the gpu when its loaded.
  uint32_t* texture_ids;
} render_data_t;

void
free_render_data(
  render_data_t* render_data, 
  const allocator_t* allocator);

render_data_t* 
load_scene_render_data(
  scene_t* scene, 
  const allocator_t* allocator);

render_data_t*
load_mesh_renderer_data(
  mesh_t* mesh, 
  const color_rgba_t color, 
  const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif