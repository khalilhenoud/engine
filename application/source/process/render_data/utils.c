/**
 * @file utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <library/string/string.h>
#include <application/process/render_data/utils.h>
#include <application/converters/to_render_data.h>
#include <entity/c/runtime/texture.h>
#include <entity/c/runtime/texture_utils.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/light.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <math/c/matrix4f.h>


// note: this function loads the textures from disk and upload the texture data
// to the gpu. it does the same thing for the font texture.
void
prep_packaged_render_data(
  const char* data_set,
  const char* folder,
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator)
{
  char texture_path[1024] = { 0 };
  snprintf(
    texture_path, sizeof(texture_path), "%s\\%s\\textures\\", data_set, folder);

  // load the images and upload them to the gpu.
  for (uint32_t i = 0; i < render_data->mesh_data.count; ++i) {
    texture_runtime_t* runtime = render_data->mesh_data.texture_runtimes + i;
    if (runtime->texture.path && runtime->texture.path->size) {
      load_image_buffer(texture_path, runtime, allocator);
      render_data->mesh_data.texture_ids[i] = upload_to_gpu(
        runtime->texture.path->str,
        runtime->buffer,
        runtime->width,
        runtime->height,
        (renderer_image_format_t)runtime->format);
    }
  }

  // do the same for the fonts.
  for (uint32_t i = 0; i < render_data->font_data.count; ++i) {
    load_font_inplace(
      data_set, 
      &render_data->font_data.fonts[i].font,
      &render_data->font_data.fonts[i], 
      allocator);
    // TODO: Ultimately we need a system for this, we need to be able to handle
    // deduplication.
    texture_runtime_t* runtime = render_data->font_data.texture_runtimes + i;
    if (runtime->texture.path && runtime->texture.path->size) {
      load_image_buffer(data_set, runtime, allocator);
      render_data->font_data.texture_ids[i] = upload_to_gpu(
        runtime->texture.path->str,
        runtime->buffer,
        runtime->width,
        runtime->height,
        (renderer_image_format_t)runtime->format);
    }
  }

  for (uint32_t i = 0; i < render_data->light_data.count; ++i)
    enable_light(i);
}

void
cleanup_packaged_render_data(
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator)
{
  for (uint32_t i = 0; i < render_data->mesh_data.count; ++i) {
    if (render_data->mesh_data.texture_ids[i])
      evict_from_gpu(render_data->mesh_data.texture_ids[i]);
  }

  for (uint32_t i = 0; i < render_data->font_data.count; ++i) {
    if (render_data->font_data.texture_ids[i])
      evict_from_gpu(render_data->font_data.texture_ids[i]);
  }

  for (uint32_t i = 0; i < render_data->light_data.count; ++i)
    disable_light(i);

  free_render_data(render_data, allocator);
}

static
void
render_packaged_scene_data_node(
  packaged_scene_render_data_t* render_data,
  pipeline_t* pipeline,
  node_t* node)
{
  // render the meshes before recursively calling itself.
  push_matrix(pipeline);
  pre_multiply(pipeline, &node->transform);

  {
    // draw the meshes belonging to this node.
    for (uint32_t i = 0; i < node->meshes.count; ++i) {
      uint32_t mesh_index = node->meshes.indices[i];
      draw_meshes(
        render_data->mesh_data.mesh_render_data + mesh_index,
        render_data->mesh_data.texture_ids + mesh_index,
        1,
        pipeline);
    }

    // recurively call the child nodes.
    for (uint32_t i = 0; i < node->nodes.count; ++i) {
      uint32_t node_index = node->nodes.indices[i];
      render_packaged_scene_data_node(
        render_data, 
        pipeline, 
        render_data->node_data.nodes + node_index);
    }
  }

  pop_matrix(pipeline);
}

static
void
set_packaged_light_properties(
  camera_t* camera,
  packaged_scene_render_data_t* render_data, 
  pipeline_t* pipeline)
{
  for (uint32_t i = 0; i < render_data->light_data.count; ++i) {
    renderer_light_t* light = render_data->light_data.lights + i;
    set_light_properties(i, light, pipeline);
  }

  //// get the 8 closest light sources and use those.
  //float distance = 10000000.f;
  //float used_diff[8]; 
  //uint32_t used[8];
  //uint32_t count = 0;
  //vector3f dis;
  //float disf;

  //for (uint32_t i = 0; i < 8; ++i) {
  //  used_diff[i] = distance;
  //  used[i] = 0;
  //}

  //for (uint32_t i = 0; i < render_data->light_data.count; ++i) {
  //  renderer_light_t* light = render_data->light_data.lights + i;
  //  vector3f_set_diff_v3f(&dis, &light->position, &camera->position);
  //  disf = length_v3f(&dis);
  //  for (int32_t k = 7; k >= 0; --k) {
  //    if (used_diff[k] > disf) {
  //      if (k != 7) {
  //        used_diff[k + 1] = used_diff[k];
  //        used[k + 1] = used[k];
  //      }
  //      used_diff[k] = disf;
  //      used[k] = i;
  //    }
  //  }
  //}

  //for (uint32_t i = 0, last_used = 1000000; i < 8; ++i) {
  //  if (last_used == used[i])
  //    break;
  //  last_used = used[i];
  //  renderer_light_t* light = render_data->light_data.lights + used[i];
  //  light->ambient.data[0] = light->ambient.data[1] = light->ambient.data[2] = 0.f;
  //  light->diffuse.data[0] = light->diffuse.data[1] = light->diffuse.data[2] = 1.f;
  //  light->attenuation_constant = 1;
  //  light->attenuation_linear = 0.01;

  //  set_light_properties(i, light, pipeline);
  //}
}

void
render_packaged_scene_data(
  packaged_scene_render_data_t* render_data,
  pipeline_t* pipeline,
  camera_t* camera)
{
  assert(render_data && pipeline && camera);
  assert(
    render_data->node_data.count > 0 && 
    "at least the root node must exist!");

  {
    matrix4f out;
    memset(&out, 0, sizeof(matrix4f));
    get_view_transformation(camera, &out);
    set_matrix_mode(pipeline, MODELVIEW);
    load_identity(pipeline);
    post_multiply(pipeline, &out);

    set_packaged_light_properties(camera, render_data, pipeline);

    // render the root node.
    render_packaged_scene_data_node(
      render_data, 
      pipeline, 
      render_data->node_data.nodes + 0);
  }
}