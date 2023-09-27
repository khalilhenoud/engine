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
#include <application/converters/to_render_data.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/node_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/mesh/material.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/color.h>
#include <renderer/renderer_opengl_data.h>


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

// TODO: Move the renderer data create/free to the renderer package itself.
void
free_render_data(
  render_data_t* render_data, 
  const allocator_t* allocator)
{
  assert(render_data && allocator);
  
  {
    free_mesh_render_data_array(
      render_data->mesh_render_data, render_data->mesh_count, allocator);

    allocator->mem_free(render_data->textures);
    allocator->mem_free(render_data->texture_ids);
    allocator->mem_free(render_data);
  }
}

render_data_t* 
load_scene_render_data(
  scene_t* scene, 
  const allocator_t* allocator)
{
  assert(scene && allocator);

  {
    render_data_t* render_data = 
      allocator->mem_alloc(sizeof(render_data_t));
    
    // We do match the arrays size between meshes and textures.
    render_data->mesh_count = 
    render_data->texture_count = scene->mesh_repo.count;

    render_data->mesh_render_data = 
      allocator->mem_cont_alloc(
        render_data->mesh_count, 
        sizeof(mesh_render_data_t));

    render_data->textures = 
      allocator->mem_cont_alloc(
        render_data->texture_count, 
        sizeof(fixed_str_t));

    render_data->texture_ids = 
      allocator->mem_cont_alloc(
        render_data->texture_count, 
        sizeof(uint32_t));
    memset(
      render_data->texture_ids, 
      0, 
      sizeof(uint32_t) * render_data->texture_count);

    for (uint32_t i = 0; i < scene->mesh_repo.count; ++i) {
      mesh_t* mesh = scene->mesh_repo.meshes + i;
      mesh_render_data_t* r_data = render_data->mesh_render_data + i;
      
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
      memset(
        render_data->textures[i].data, 
        0, 
        sizeof(render_data->textures[i].data));
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
          memcpy(
            render_data->textures[i].data, 
            texture->path.data, 
            sizeof(render_data->textures[i].data));
        }
      }
    }
    
    return render_data;
  }
}

render_data_t*
load_mesh_renderer_data(
  mesh_t* mesh, 
  const color_rgba_t color, 
  const allocator_t* allocator)
{
  assert(mesh && allocator);

  {
    render_data_t* render_data = allocator->mem_alloc(sizeof(render_data_t));
    
    // We do match the arrays size between meshes and textures.
    render_data->mesh_count = render_data->texture_count = 1;

    render_data->mesh_render_data = 
      allocator->mem_cont_alloc(
        render_data->mesh_count, 
        sizeof(mesh_render_data_t));

    render_data->textures = 
      allocator->mem_cont_alloc(
        render_data->texture_count, 
        sizeof(fixed_str_t));
    
    render_data->texture_ids = 
      allocator->mem_cont_alloc(
        render_data->texture_count, 
        sizeof(uint32_t));
    memset(
      render_data->texture_ids, 
      0, 
      sizeof(uint32_t) * render_data->texture_count);
      
    {
      mesh_render_data_t* r_data = render_data->mesh_render_data;
      
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

      memset(
        render_data->textures[0].data, 
        0, 
        sizeof(render_data->textures[0].data));

      array_size = sizeof(r_data->diffuse.data);
      memcpy(r_data->ambient.data, color.data, array_size);
      memcpy(r_data->diffuse.data, color.data, array_size);
      memcpy(r_data->specular.data, color.data, array_size);
    }
    
    return render_data;
  }
}