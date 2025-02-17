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
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <application/converters/to_render_data.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/node_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/light.h>
#include <entity/c/scene/light_utils.h>
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


static
cstring_t *
allocate_string(const char *ptr, const allocator_t *allocator)
{
  cstring_t *string = allocator->mem_alloc(sizeof(cstring_t));
  cstring_def(string);
  cstring_setup(string, ptr, allocator);
  return string;
}

static
void
free_string(cstring_t *string, const allocator_t *allocator)
{
  cstring_cleanup(string, NULL);
  allocator->mem_free(string);
}

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
      free_string(current->name, allocator);
      allocator->mem_free(current->nodes.indices);
      allocator->mem_free(current->meshes.indices);
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
  mesh_data->count = scene->mesh_repo.count;

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

  for (uint32_t i = 0; i < scene->mesh_repo.count; ++i) {
    mesh_t* mesh = scene->mesh_repo.meshes + i;
    mesh_render_data_t* r_data = mesh_data->mesh_render_data + i;
    
    uint32_t array_size = sizeof(float) * 3 * mesh->vertices_count;
    r_data->vertex_count = mesh->vertices_count;
    r_data->vertices = allocator->mem_alloc(array_size);
    memcpy(r_data->vertices, mesh->vertices, array_size);
    r_data->normals = allocator->mem_alloc(array_size);
    memcpy(r_data->normals, mesh->normals, array_size);
    r_data->uv_coords = allocator->mem_alloc(array_size);
    memcpy(r_data->uv_coords, mesh->uvs, array_size);

    array_size = sizeof(uint32_t) * mesh->indices_count;
    r_data->indices_count = mesh->indices_count;
    r_data->indices = allocator->mem_alloc(array_size);
    memcpy(r_data->indices, mesh->indices, array_size);

    // Set the default texture and material colors to grey.
    mesh_data->texture_runtimes[i].texture.path = NULL;
    // set a default ambient color.
    r_data->ambient.data[0] = 
    r_data->ambient.data[1] = r_data->ambient.data[2] = 0.5f;
    r_data->ambient.data[3] = 1.f;
    // copy the ambient default color into the diffuse and specular.
    array_size = sizeof(r_data->diffuse.data);
    memcpy(r_data->diffuse.data, r_data->ambient.data, array_size);
    memcpy(r_data->specular.data, r_data->ambient.data, array_size);

    if (mesh->materials.used) {
      material_t* mat = 
        scene->material_repo.materials + mesh->materials.indices[0];
      // the types are compatible float_4, so copying is safe.
      size_t size = sizeof(r_data->ambient.data);
      memcpy(r_data->ambient.data, mat->ambient.data, size);
      memcpy(r_data->diffuse.data, mat->diffuse.data, size);
      memcpy(r_data->specular.data, mat->specular.data, size);

      if (mat->textures.used) {
        texture_t* texture = 
          scene->texture_repo.textures + mat->textures.data->index;
        mesh_data->texture_runtimes[i].texture.path = 
          allocate_string(texture->path->str, allocator);
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
  font_data->count = scene->font_repo.count;
  font_data->fonts = 
    (font_runtime_t*)allocator->mem_cont_alloc(
      scene->font_repo.count, sizeof(font_runtime_t));
  font_data->texture_runtimes = 
    (texture_runtime_t*)allocator->mem_cont_alloc(
      scene->font_repo.count, 
      sizeof(texture_runtime_t));
  
  // the texture ids.
  font_data->texture_ids = 
    (uint32_t*)allocator->mem_cont_alloc(
      scene->font_repo.count, sizeof(uint32_t));
  memset(
    font_data->texture_ids, 
    0, 
    sizeof(uint32_t) * scene->font_repo.count);

  for (uint32_t i = 0; i < scene->font_repo.count; ++i) {
    font_runtime_t* target = font_data->fonts + i;
    texture_runtime_t* target_image = font_data->texture_runtimes + i;
    font_t* source = scene->font_repo.fonts + i;

    target->font.data_file = 
      allocate_string(source->data_file->str, allocator);
    target->font.image_file = 
      allocate_string(source->image_file->str, allocator);

    // Set the texture runtime of the font, still unloaded.
    target_image->texture.path = 
      allocate_string(source->image_file->str, allocator);
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
    light_data->count = scene->light_repo.count;
    light_data->lights = 
      (renderer_light_t*)allocator->mem_cont_alloc(
        light_data->count, sizeof(renderer_light_t));

    for (uint32_t i = 0; i < light_data->count; ++i) {
      renderer_light_t* target = light_data->lights + i;
      size_t size_color = sizeof(target->diffuse.data);
      size_t size_vector = sizeof(target->position.data);
      light_t* source = scene->light_repo.lights + i;

      target->attenuation_constant = source->attenuation_constant;
      target->attenuation_linear = source->attenuation_linear;
      target->attenuation_quadratic = source->attenuation_quadratic;
      target->inner_cone = source->inner_cone;
      target->outer_cone = source->outer_cone;
      target->type = source->type;
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
    camera_data->count = scene->camera_repo.count;
    camera_data->cameras = 
      (camera_t*)allocator->mem_cont_alloc(
        scene->camera_repo.count, sizeof(camera_t));

    for (uint32_t i = 0; i < camera_data->count; ++i) {
      camera_t* target = camera_data->cameras + i;
      camera_t* source = scene->camera_repo.cameras + i;

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
    node_data->count = scene->node_repo.count;
    node_data->nodes = 
      (node_t*)allocator->mem_cont_alloc(
        scene->node_repo.count, sizeof(node_t));

    for (uint32_t i = 0; i < node_data->count; ++i) {
      node_t* target = node_data->nodes + i;
      node_t* source = scene->node_repo.nodes + i;

      // copy the name and the matrix.
      target->name = allocate_string(source->name->str, allocator);
      memcpy(
        target->transform.data, 
        source->transform.data, 
        sizeof(target->transform.data));

      // copy the children node indices.
      target->nodes.count = source->nodes.count;
      target->nodes.indices = (uint32_t*)allocator->mem_cont_alloc(
        target->nodes.count, sizeof(uint32_t));
      memcpy(
        target->nodes.indices, 
        source->nodes.indices, 
        sizeof(uint32_t) * target->nodes.count);

      // copy the mesh payload indices.
      target->meshes.count = source->meshes.count;
      target->meshes.indices = (uint32_t*)allocator->mem_cont_alloc(
        target->meshes.count, sizeof(uint32_t));
      memcpy(
        target->meshes.indices, 
        source->meshes.indices, 
        sizeof(uint32_t) * target->meshes.count);
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
    
    uint32_t array_size = sizeof(float) * 3 * mesh->vertices_count;
    r_data->vertex_count = mesh->vertices_count;
    r_data->vertices = allocator->mem_alloc(array_size);
    memcpy(r_data->vertices, mesh->vertices, array_size);
    r_data->normals = allocator->mem_alloc(array_size);
    memcpy(r_data->normals, mesh->normals, array_size);
    r_data->uv_coords = allocator->mem_alloc(array_size);
    memcpy(r_data->uv_coords, mesh->uvs, array_size);

    array_size = sizeof(uint32_t) * mesh->indices_count;
    r_data->indices_count = mesh->indices_count;
    r_data->indices = allocator->mem_alloc(array_size);
    memcpy(r_data->indices, mesh->indices, array_size);

    // Set the default texture and material colors to grey.
    mesh_data->texture_runtimes[0].texture.path = NULL;
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
