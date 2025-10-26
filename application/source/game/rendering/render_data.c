/**
 * @file to_render_data.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-09-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <application/game/rendering/render_data.h>
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/light.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/mesh/material.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/color.h>
#include <entity/c/runtime/texture.h>
#include <entity/c/runtime/texture_utils.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <renderer/renderer_opengl_data.h>
#include <renderer/pipeline.h>


static
void
free_mesh_render_data_internal(
  mesh_render_data_t* render_data, 
  const allocator_t* allocator)
{
  assert(render_data && allocator);

  allocator->mem_free(render_data->vertices);
  allocator->mem_free(render_data->normals);
  allocator->mem_free(render_data->uv_coords);
  allocator->mem_free(render_data->indices);
}

static
void
free_mesh_render_data_array(
  mesh_render_data_t* meshes_data, 
  uint32_t count, 
  const allocator_t* allocator)
{
  assert(meshes_data && count && allocator);
  
  {
    for (uint32_t i = 0; i < count; ++i)
      free_mesh_render_data_internal(meshes_data + i, allocator);
    
    allocator->mem_free(meshes_data);
  }
}

static
void
free_font_runtime_array(
  font_runtime_t* runtime, 
  uint32_t count, 
  const allocator_t* allocator)
{
  assert(runtime && count && allocator);

  {
    for (uint32_t i = 0; i < count; ++i)
      free_font_runtime_internal(runtime + i, allocator);

    allocator->mem_free(runtime);
  }
}

static
void
free_texture_runtime_array(
  texture_runtime_t* runtime, 
  uint32_t count, 
  const allocator_t* allocator)
{
  assert(runtime && count && allocator);

  {
    for (uint32_t i = 0; i < count; ++i)
      free_texture_runtime_internal(runtime + i, allocator);

    allocator->mem_free(runtime);
  }
}

static
void
free_packaged_mesh_data_internal(
  packaged_mesh_data_t* mesh_data, 
  const allocator_t* allocator)
{
  assert(mesh_data && allocator);

  free_mesh_render_data_array(
    mesh_data->mesh_render_data, 
    mesh_data->count, 
    allocator);

  free_texture_runtime_array(
    mesh_data->texture_runtimes, 
    mesh_data->count, 
    allocator);

  allocator->mem_free(mesh_data->texture_ids);  
}

void
free_mesh_render_data(
  packaged_mesh_data_t* mesh_data, 
  const allocator_t* allocator)
{
  assert(mesh_data && allocator);

  free_packaged_mesh_data_internal(mesh_data, allocator);
  allocator->mem_free(mesh_data);
}

static
void
free_packaged_font_data_internal(
  packaged_font_data_t* font_data,
  const allocator_t* allocator)
{
  assert(font_data && allocator);
  
  free_font_runtime_array(
    font_data->fonts, 
    font_data->count, 
    allocator);
  
  free_texture_runtime_array(
    font_data->texture_runtimes, 
    font_data->count, 
    allocator);

  allocator->mem_free(font_data->texture_ids);
}

static
void
free_packaged_camera_data_internal(
  packaged_camera_data_t* camera_data,
  const allocator_t* allocator)
{
  assert(camera_data && allocator);

  allocator->mem_free(camera_data->cameras);
}

static
void
free_packaged_light_data_internal(
  packaged_light_data_t* light_data, 
  const allocator_t* allocator)
{
  assert(light_data && allocator);

  allocator->mem_free(light_data->lights);
}

static
void
free_packaged_node_data_internal(
  packaged_node_data_t* node_data, 
  const allocator_t* allocator)
{
  assert(node_data && allocator);

  {
    for (uint32_t i = 0; i < node_data->count; ++i) {
      node_t* current = node_data->nodes + i;
      cstring_cleanup2(&current->name);
      cvector_cleanup2(&current->nodes);
      cvector_cleanup2(&current->meshes);
    }

    allocator->mem_free(node_data->nodes);
  }
}

void
free_render_data(
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator)
{
  assert(render_data && allocator);
  
  {
    free_packaged_node_data_internal(&render_data->node_data, allocator);
    free_packaged_mesh_data_internal(&render_data->mesh_data, allocator);
    free_packaged_light_data_internal(&render_data->light_data, allocator);
    free_packaged_font_data_internal(&render_data->font_data, allocator);
    free_packaged_camera_data_internal(&render_data->camera_data, allocator);  
    allocator->mem_free(render_data);
  }
}

static
void
load_scene_mesh_data(
  scene_t* scene, 
  packaged_mesh_data_t* mesh_data, 
  const allocator_t* allocator)
{
  assert(scene && mesh_data && allocator);

  // We do match the arrays size between meshes and textures.
  mesh_data->count = scene->mesh_repo.size;

  // mesh render data.
  mesh_data->mesh_render_data = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(mesh_render_data_t));

  // texture render data.
  mesh_data->texture_runtimes = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(texture_runtime_t));

  // fill these when you load the texture runtimes.
  mesh_data->texture_ids = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(uint32_t));
  memset(
    mesh_data->texture_ids, 
    0, 
    sizeof(uint32_t) * mesh_data->count);

  for (uint32_t i = 0; i < scene->mesh_repo.size; ++i) {
    mesh_t* mesh = cvector_as(&scene->mesh_repo, i, mesh_t);
    mesh_render_data_t* r_data = mesh_data->mesh_render_data + i;
    
    uint32_t array_size = sizeof(float) * mesh->vertices.size;
    r_data->vertex_count = (mesh->vertices.size)/3;
    r_data->vertices = allocator->mem_alloc(array_size);
    memcpy(r_data->vertices, mesh->vertices.data, array_size);
    r_data->normals = allocator->mem_alloc(array_size);
    memcpy(r_data->normals, mesh->normals.data, array_size);
    r_data->uv_coords = allocator->mem_alloc(array_size);
    memcpy(r_data->uv_coords, mesh->uvs.data, array_size);

    array_size = sizeof(uint32_t) * mesh->indices.size;
    r_data->indices_count = mesh->indices.size;
    r_data->indices = allocator->mem_alloc(array_size);
    memcpy(r_data->indices, mesh->indices.data, array_size);

    // Set the default texture and material colors to grey.
    cstring_def(&mesh_data->texture_runtimes[i].texture.path);
    // set a default ambient color.
    r_data->ambient.data[0] = 
    r_data->ambient.data[1] = r_data->ambient.data[2] = 0.5f;
    r_data->ambient.data[3] = 1.f;
    // copy the ambient default color into the diffuse and specular.
    array_size = sizeof(r_data->diffuse.data);
    memcpy(r_data->diffuse.data, r_data->ambient.data, array_size);
    memcpy(r_data->specular.data, r_data->ambient.data, array_size);

    if (mesh->materials.used) {
      material_t* mat = cvector_as(
        &scene->material_repo, mesh->materials.indices[0], material_t);
      // the types are compatible float_4, so copying is safe.
      size_t size = sizeof(r_data->ambient.data);
      memcpy(r_data->ambient.data, mat->ambient.data, size);
      memcpy(r_data->diffuse.data, mat->diffuse.data, size);
      memcpy(r_data->specular.data, mat->specular.data, size);

      if (mat->textures.used) {
        texture_t* texture = cvector_as(
          &scene->texture_repo, mat->textures.data->index, texture_t);
        cstring_def(&mesh_data->texture_runtimes[i].texture.path);
        cstring_setup(
          &mesh_data->texture_runtimes[i].texture.path, 
          texture->path.str, allocator);
      }
    }
  }
}

static
void
load_scene_font_data(
  scene_t* scene, 
  packaged_font_data_t* font_data, 
  const allocator_t* allocator)
{
  assert(scene && font_data && allocator);

  // font render data.
  font_data->count = scene->font_repo.size;
  font_data->fonts = 
    (font_runtime_t*)allocator->mem_cont_alloc(
      scene->font_repo.size, sizeof(font_runtime_t));
  font_data->texture_runtimes = 
    (texture_runtime_t*)allocator->mem_cont_alloc(
      scene->font_repo.size, 
      sizeof(texture_runtime_t));
  
  // the texture ids.
  font_data->texture_ids = 
    (uint32_t*)allocator->mem_cont_alloc(
      scene->font_repo.size, sizeof(uint32_t));
  memset(
    font_data->texture_ids, 
    0, 
    sizeof(uint32_t) * scene->font_repo.size);

  for (uint32_t i = 0; i < scene->font_repo.size; ++i) {
    font_runtime_t* target = font_data->fonts + i;
    texture_runtime_t* target_image = font_data->texture_runtimes + i;
    font_t *source = cvector_as(&scene->font_repo, i, font_t);

    cstring_def(&target->font.data_file);
    cstring_setup(&target->font.data_file, source->data_file.str, allocator);
    cstring_def(&target->font.image_file);
    cstring_setup(&target->font.image_file, source->image_file.str, allocator);

    // Set the texture runtime of the font, still unloaded.
    cstring_def(&target_image->texture.path);
    cstring_setup(
      &target_image->texture.path, source->image_file.str, allocator);
  }
}

static
void
normalize_color(color_t* color)
{
  float val = 0.f;
  for (uint32_t i = 0; i < 3; ++i)
    val += color->data[i] * color->data[i];
  val = sqrtf(val);
  if (!IS_ZERO_MP(val)) {
    for (uint32_t i = 0; i < 3; ++i)
      color->data[i] /= val;
  }
}

static
void
load_scene_light_data(
  scene_t* scene, 
  packaged_light_data_t* light_data, 
  const allocator_t* allocator)
{
  assert(scene && light_data && allocator);

  {
    light_data->count = scene->light_repo.size;
    light_data->lights = 
      (renderer_light_t*)allocator->mem_cont_alloc(
        light_data->count, sizeof(renderer_light_t));

    for (uint32_t i = 0; i < light_data->count; ++i) {
      renderer_light_t* target = light_data->lights + i;
      size_t size_color = sizeof(target->diffuse.data);
      size_t size_vector = sizeof(target->position.data);
      light_t* source = cvector_as(&scene->light_repo, i, light_t);

      target->attenuation_constant = source->attenuation_constant;
      target->attenuation_linear = source->attenuation_linear;
      target->attenuation_quadratic = source->attenuation_quadratic;
      target->inner_cone = source->inner_cone;
      target->outer_cone = source->outer_cone;
      target->type = (renderer_light_type_t)source->type;
      memcpy(target->diffuse.data, source->diffuse.data, size_color);
      memcpy(target->specular.data, source->specular.data, size_color);
      memcpy(target->ambient.data, source->ambient.data, size_color);
      normalize_color(&target->diffuse);
      normalize_color(&target->specular);
      normalize_color(&target->ambient);
      memcpy(target->position.data, source->position.data, size_vector);
      memcpy(target->direction.data, source->direction.data, size_vector);
      memcpy(target->up.data, source->up.data, size_vector);
    }
  }
}

static
void
load_scene_camera_data(
  scene_t* scene, 
  packaged_camera_data_t* camera_data, 
  const allocator_t* allocator)
{
  assert(scene && camera_data && allocator);

  {
    camera_data->count = scene->camera_repo.size;
    camera_data->cameras = 
      (camera_t*)allocator->mem_cont_alloc(
        scene->camera_repo.size, sizeof(camera_t));

    for (uint32_t i = 0; i < camera_data->count; ++i) {
      camera_t* target = camera_data->cameras + i;
      camera_t* source = cvector_as(&scene->camera_repo, i, camera_t);

      target->position = source->position;
      target->lookat_direction = source->lookat_direction;
      target->up_vector = source->up_vector;
    }
  } 
}

static
void
load_scene_node_data(
  scene_t* scene, 
  packaged_node_data_t* node_data, 
  const allocator_t* allocator)
{
  assert(scene && node_data && allocator);

  {
    node_data->count = scene->node_repo.size;
    node_data->nodes = 
      (node_t*)allocator->mem_cont_alloc(scene->node_repo.size, sizeof(node_t));

    for (uint32_t i = 0; i < node_data->count; ++i) {
      node_t *target = node_data->nodes + i;
      node_t *source = cvector_as(&scene->node_repo, i, node_t);

      // copy the name and the matrix.
      cstring_def(&target->name);
      cstring_setup(&target->name, source->name.str, allocator);
      memcpy(
        target->transform.data, 
        source->transform.data, 
        sizeof(target->transform.data));

      // copy the children node indices.
      cvector_setup(&target->nodes, get_type_data(uint32_t), 0, allocator);
      cvector_resize(&target->nodes, source->nodes.size);
      memcpy(
        target->nodes.data, 
        source->nodes.data, 
        sizeof(uint32_t) * target->nodes.size);

      // copy the mesh payload indices.
      cvector_setup(&target->meshes, get_type_data(uint32_t), 0, allocator);
      cvector_resize(&target->meshes, source->meshes.size);
      memcpy(
        target->meshes.data, 
        source->meshes.data, 
        sizeof(uint32_t) * target->meshes.size);
    }
  }
}

// TODO: Right now this is limited to a single texture. Improve this.
packaged_scene_render_data_t* 
load_scene_render_data(
  scene_t* scene, 
  const allocator_t* allocator)
{
  assert(scene && allocator);

  {
    packaged_scene_render_data_t* render_data = 
      allocator->mem_alloc(sizeof(packaged_scene_render_data_t));
    memset(render_data, 0, sizeof(packaged_scene_render_data_t));

    load_scene_node_data(scene, &render_data->node_data, allocator);
    load_scene_mesh_data(scene, &render_data->mesh_data, allocator);
    load_scene_light_data(scene, &render_data->light_data, allocator);
    load_scene_font_data(scene, &render_data->font_data, allocator);
    load_scene_camera_data(scene, &render_data->camera_data, allocator);
    
    return render_data;
  }
}

packaged_mesh_data_t*
load_mesh_renderer_data(
  mesh_t* mesh, 
  const color_rgba_t color, 
  const allocator_t* allocator)
{
  assert(mesh && allocator);

  packaged_mesh_data_t* mesh_data = 
    allocator->mem_alloc(sizeof(packaged_mesh_data_t));

  mesh_data->count = 1;

  // mesh render data.
  mesh_data->mesh_render_data = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(mesh_render_data_t));

  // texture render data.
  mesh_data->texture_runtimes = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(texture_runtime_t));

  // fill these when you load the texture runtimes.
  mesh_data->texture_ids = 
    allocator->mem_cont_alloc(
      mesh_data->count, 
      sizeof(uint32_t));
  memset(
    mesh_data->texture_ids, 
    0, 
    sizeof(uint32_t) * mesh_data->count);

  {
    mesh_render_data_t* r_data = mesh_data->mesh_render_data;
    
    uint32_t array_size = sizeof(float) * mesh->vertices.size;
    r_data->vertex_count = (mesh->vertices.size)/3;
    r_data->vertices = allocator->mem_alloc(array_size);
    memcpy(r_data->vertices, mesh->vertices.data, array_size);
    r_data->normals = allocator->mem_alloc(array_size);
    memcpy(r_data->normals, mesh->normals.data, array_size);
    r_data->uv_coords = allocator->mem_alloc(array_size);
    memcpy(r_data->uv_coords, mesh->uvs.data, array_size);

    array_size = sizeof(uint32_t) * mesh->indices.size;
    r_data->indices_count = mesh->indices.size;
    r_data->indices = allocator->mem_alloc(array_size);
    memcpy(r_data->indices, mesh->indices.data, array_size);

    // Set the default texture and material colors to grey.
    cstring_def(&mesh_data->texture_runtimes[0].texture.path);
    cstring_setup(&mesh_data->texture_runtimes[0].texture.path, "", allocator);
    // set a default ambient color.
    r_data->ambient.data[0] = color.data[0];
    r_data->ambient.data[1] = color.data[1];
    r_data->ambient.data[2] = color.data[2];
    r_data->ambient.data[3] = color.data[3];
    // copy the ambient default color into the diffuse and specular.
    array_size = sizeof(r_data->diffuse.data);
    memcpy(r_data->diffuse.data, r_data->ambient.data, array_size);
    memcpy(r_data->specular.data, r_data->ambient.data, array_size);
  }

  return mesh_data;
}

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
    if (runtime->texture.path.str && runtime->texture.path.length) {
      load_image_buffer(texture_path, runtime, allocator);
      render_data->mesh_data.texture_ids[i] = upload_to_gpu(
        runtime->texture.path.str,
        runtime->buffer.data,
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
    if (runtime->texture.path.str && runtime->texture.path.length) {
      load_image_buffer(data_set, runtime, allocator);
      render_data->font_data.texture_ids[i] = upload_to_gpu(
        runtime->texture.path.str,
        runtime->buffer.data,
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
    for (uint32_t i = 0; i < node->meshes.size; ++i) {
      uint32_t mesh_index = *cvector_as(&node->meshes, i, uint32_t);
      draw_meshes(
        render_data->mesh_data.mesh_render_data + mesh_index,
        render_data->mesh_data.texture_ids + mesh_index,
        1,
        pipeline);
    }

    // recurively call the child nodes.
    for (uint32_t i = 0; i < node->nodes.size; ++i) {
      uint32_t node_index = *cvector_as(&node->nodes, i, uint32_t);
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
#if 0
  for (uint32_t i = 0; i < render_data->light_data.count; ++i) {
    renderer_light_t* light = render_data->light_data.lights + i;
    set_light_properties(i, light, pipeline);
  }
#else
  renderer_light_t light;
  light.type = RENDERER_LIGHT_TYPE_DIRECTIONAL;
  light.position.data[0] = 0;
  light.position.data[1] = 1;
  light.position.data[2] = 0;
  light.direction.data[0] = 0;
  light.direction.data[1] = 0;
  light.direction.data[2] = 0;
  light.up.data[0] = 0;
  light.up.data[1] = 0;
  light.up.data[2] = 0;
  light.inner_cone = 0;
  light.outer_cone = 0;
  light.attenuation_constant = 1;
  light.attenuation_linear = 0.001f;
  light.attenuation_quadratic = 0;
  light.ambient.data[0] = 
    light.ambient.data[1] = 
    light.ambient.data[2] = 
    light.ambient.data[3] = 1.f;
  light.diffuse.data[0] =
    light.diffuse.data[1] =
    light.diffuse.data[2] =
    light.diffuse.data[3] = 1.f;
  light.specular.data[0] =
    light.specular.data[1] =
    light.specular.data[2] = 0.f;
  light.specular.data[3] = 1.f;
  set_light_properties(0, &light, pipeline);
  light.position.data[0] = 1;
  light.position.data[1] = 0;
  light.position.data[2] = 0;
  light.ambient.data[0] =
  light.ambient.data[1] =
  light.ambient.data[2] = 0.2f;
  set_light_properties(1, &light, pipeline);
#endif
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
    camera_view_matrix(camera, &out);
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