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
#include <library/containers/cvector.h>
#include <entity/c/mesh/color.h>


// NOTE: This is all extremely rudimentary until the time when I implement a 
// scene graph module that can replace this.

typedef struct scene_t scene_t;
typedef struct mesh_t mesh_t;
typedef struct allocator_t allocator_t;
typedef struct camera_t camera_t;
typedef struct pipeline_t pipeline_t;

typedef
struct packaged_mesh_data_t {
  cvector_t mesh_render_data;           // mesh_render_data_t
  cvector_t texture_runtimes;           // texture_runtime_t
  cvector_t texture_ids;                // uint32_t
} packaged_mesh_data_t;

typedef
struct packaged_font_data_t {
  cvector_t fonts;                      // font_runtime_t
  cvector_t texture_runtimes;           // texture_runtime_t
  cvector_t texture_ids;                // uint32_t
} packaged_font_data_t;

// NOTE: a node has a transform and a list of packaged mesh or other nodes.
// these are indices into the main packaged_scene_render_data_t struct.
typedef
struct packaged_scene_render_data_t {
  cvector_t node_data;                    // node_t
  packaged_mesh_data_t mesh_data;
  packaged_font_data_t font_data;
  cvector_t light_data;                   // renderer_light_t
  cvector_t camera_data;                  // camera_t
} packaged_scene_render_data_t;

void
free_render_data(
  packaged_scene_render_data_t *render_data, 
  const allocator_t *allocator);

void
free_mesh_render_data(
  packaged_mesh_data_t *mesh_data, 
  const allocator_t *allocator);

packaged_scene_render_data_t * 
load_scene_render_data(
  scene_t *scene, 
  const allocator_t *allocator);

packaged_mesh_data_t *
load_mesh_renderer_data(
  mesh_t *mesh, 
  const color_rgba_t color, 
  const allocator_t *allocator);

void
prep_packaged_render_data(
  const char *data_set,
  const char *folder,
  packaged_scene_render_data_t *render_data, 
  const allocator_t *allocator);

void
cleanup_packaged_render_data(
  packaged_scene_render_data_t *render_data, 
  const allocator_t *allocator);

void
render_packaged_scene_data(
  packaged_scene_render_data_t *render_data,
  pipeline_t *pipeline,
  camera_t *camera);

#ifdef __cplusplus
}
#endif

#endif