/**
 * @file bin_to_scene.c
 * @author khalilhenoud@gmail.com
 * @brief convert a bin scene into an entity scene.
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <library/allocator/allocator.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/node_utils.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/material.h>
#include <entity/c/mesh/color.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <serializer/serializer_scene_data.h>
#include <application/converters/bin_to_scene_to_bin.h>


static
void
populate_serializer_texture_data(
  scene_t* scene, 
  serializer_scene_data_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->texture_repo.used = scene->texture_repo.count;
    if (target->texture_repo.used) {
      target->texture_repo.data = 
        allocator->mem_cont_alloc(
          target->texture_repo.used, 
          sizeof(serializer_texture_data_t));

      for (uint32_t i = 0; i < target->texture_repo.used; ++i) {
        memset(
          target->texture_repo.data[i].path.data, 
          0, 
          sizeof(target->texture_repo.data[i].path.data));
        memcpy(
          target->texture_repo.data[i].path.data, 
          scene->texture_repo.textures[i].path.data, 
          sizeof(target->texture_repo.data[i].path.data));
      }
    }
  }
}

static
void
copy_texture_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->texture_repo.count = scene->texture_repo.used;
    if (scene->texture_repo.used) {
      target->texture_repo.textures = 
        allocator->mem_cont_alloc(scene->texture_repo.used, sizeof(texture_t));

      for (uint32_t i = 0; i < scene->texture_repo.used; ++i) {
        memset(
          target->texture_repo.textures[i].path.data, 
          0, 
          sizeof(target->texture_repo.textures[i].path.data));
        memcpy(
          target->texture_repo.textures[i].path.data, 
          scene->texture_repo.data[i].path.data, 
          sizeof(target->texture_repo.textures[i].path.data));
      }
    }
  }
}

static
void
populate_serializer_material_data(
  scene_t* scene, 
  serializer_scene_data_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->material_repo.used = scene->material_repo.count;
    if (target->material_repo.used) {
      target->material_repo.data = 
        allocator->mem_cont_alloc(
          target->material_repo.used, 
          sizeof(serializer_material_data_t));
      
      for (uint32_t i = 0; i < target->material_repo.used; ++i) {
        serializer_material_data_t* mat = target->material_repo.data + i;
        material_t* source = scene->material_repo.materials + i;

        // will only keep 128 characters.
        memset(mat->name.data, 0, sizeof(mat->name.data));
        memcpy(mat->name.data, source->name.data, sizeof(source->name.data));
        memcpy(
          mat->ambient.data, 
          source->ambient.data, 
          sizeof(mat->ambient.data));
        memcpy(
          mat->diffuse.data, 
          source->diffuse.data, 
          sizeof(mat->diffuse.data));
        memcpy(
          mat->specular.data, 
          source->specular.data, 
          sizeof(mat->specular.data));
        mat->shininess = source->shininess;
        mat->opacity = source->opacity;

        // Only 8 textures per material are supported.
        mat->textures.used = 
          (source->textures.used > MAX_TEXTURE_COUNT_PER_MATERIAL) ?
          MAX_TEXTURE_COUNT_PER_MATERIAL : source->textures.used;

        // NOTE: we do not copy the name/type.
        for (uint32_t j = 0; j < mat->textures.used; ++j) {
          mat->textures.data[j].index   = source->textures.data[j].index;
          mat->textures.data[j].u       = source->textures.data[j].u;
          mat->textures.data[j].v       = source->textures.data[j].v;
          mat->textures.data[j].u_scale = source->textures.data[j].u_scale;
          mat->textures.data[j].v_scale = source->textures.data[j].v_scale;
          mat->textures.data[j].angle   = source->textures.data[j].angle;
        }
      }
    }
  }
}

static
void
copy_material_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->material_repo.count = scene->material_repo.used;
    if (scene->material_repo.used) {
      target->material_repo.materials = 
        allocator->mem_cont_alloc(
          scene->material_repo.used, 
          sizeof(material_t));
      
      for (uint32_t i = 0; i < scene->material_repo.used; ++i) {
        material_t* mat = target->material_repo.materials + i;
        serializer_material_data_t* source = scene->material_repo.data + i;

        // will only keep 128 characters.
        memset(mat->name.data, 0, sizeof(mat->name.data));
        memcpy(mat->name.data, source->name.data, sizeof(mat->name.data));
        memcpy(
          mat->ambient.data, 
          source->ambient.data, 
          sizeof(mat->ambient.data));
        memcpy(
          mat->diffuse.data, 
          source->diffuse.data, 
          sizeof(mat->diffuse.data));
        memcpy(
          mat->specular.data, 
          source->specular.data, 
          sizeof(mat->specular.data));
        mat->shininess = source->shininess;
        mat->opacity = source->opacity;

        // Only 8 textures per material are supported.
        mat->textures.used = 
          (source->textures.used > MAX_TEXTURE_COUNT_PER_MATERIAL) ?
          MAX_TEXTURE_COUNT_PER_MATERIAL : source->textures.used;

        // NOTE: we do not copy the name/type.
        for (uint32_t j = 0; j < mat->textures.used; ++j) {
          mat->textures.data[j].index   = source->textures.data[j].index;
          mat->textures.data[j].u       = source->textures.data[j].u;
          mat->textures.data[j].v       = source->textures.data[j].v;
          mat->textures.data[j].u_scale = source->textures.data[j].u_scale;
          mat->textures.data[j].v_scale = source->textures.data[j].v_scale;
          mat->textures.data[j].angle   = source->textures.data[j].angle;
        }
      }
    }
  }
}

static
void
populate_serializer_mesh_data(
  scene_t* scene,
  serializer_scene_data_t* target,
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  assert(scene->mesh_repo.count);

  {
    target->mesh_repo.used = scene->mesh_repo.count;
    target->mesh_repo.data = allocator->mem_cont_alloc(
      scene->mesh_repo.count, 
      sizeof(serializer_mesh_data_t));

    for (uint32_t i = 0; i < target->mesh_repo.used; ++i) {
      serializer_mesh_data_t *mesh = target->mesh_repo.data + i;
      mesh_t *source = scene->mesh_repo.meshes + i;

      // deep copy the pointers.
      uint32_t array_size = sizeof(float) * 3 * mesh->vertices_count;
      mesh->vertices_count = source->vertices_count;
      mesh->vertices = allocator->mem_alloc(array_size);
      memcpy(mesh->vertices, source->vertices, array_size);
      mesh->normals = allocator->mem_alloc(array_size);
      memcpy(mesh->normals, source->normals, array_size);
      mesh->uvs = allocator->mem_alloc(array_size);
      memcpy(mesh->uvs, source->uvs, array_size);

      array_size = sizeof(uint32_t) * source->indices_count;
      mesh->faces_count = source->indices_count / 3;
      mesh->indices = allocator->mem_alloc(array_size);
      memcpy(mesh->indices, source->indices, array_size);

      // copy the first 4 material indices, discard everything else.
      // TODO: Add logging functionality to notify the user if any materials
      // have been discarded.
      {
        mesh->materials.used = 
          (source->materials.used > MAX_MATERIAL_NUMBER) ? 
          MAX_MATERIAL_NUMBER : 
          source->materials.used;

        for (uint32_t j = 0; j < mesh->materials.used; ++j)
          mesh->materials.indices[j] = source->materials.indices[j];
      }
    }
  }
}

static
void
copy_mesh_data(
  serializer_scene_data_t* scene,
  scene_t* target,
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  assert(scene->mesh_repo.used);

  {
    target->mesh_repo.count = scene->mesh_repo.used;
    target->mesh_repo.meshes = allocate_mesh_array(
      scene->mesh_repo.used, 
      allocator);

    for (uint32_t i = 0; i < target->mesh_repo.count; ++i) {
      mesh_t *mesh = target->mesh_repo.meshes + i;
      serializer_mesh_data_t *source = scene->mesh_repo.data + i;

      // move the pointers around.
      mesh->vertices_count = source->vertices_count;
      mesh->vertices = source->vertices;
      source->vertices = NULL;
      mesh->normals = source->normals;
      source->normals = NULL;
      mesh->uvs = source->uvs;
      source->uvs = NULL;

      mesh->indices_count = source->faces_count * 3;
      mesh->indices = source->indices;
      source->indices = NULL;

      // copy the first 4 material indices, discard everything else.
      // TODO: Add logging functionality to notify the user if any materials
      // have been discarded.
      {
        mesh->materials.used = 
          (source->materials.used > MAX_MATERIAL_NUMBER) ? 
          MAX_MATERIAL_NUMBER : 
          source->materials.used;

        for (uint32_t j = 0; j < mesh->materials.used; ++j)
          mesh->materials.indices[j] = source->materials.indices[j];
      }
    }
  }
}

static
void
populate_serializer_model_data(
  scene_t* scene, 
  serializer_scene_data_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  assert(scene->node_repo.count && "at least a root node must exist!");

  {
    target->model_repo.used = scene->node_repo.count; 
    target->model_repo.data = 
      allocator->mem_cont_alloc(
        target->model_repo.used, 
        sizeof(serializer_model_data_t));
    
    for (uint32_t i = 0; i < target->model_repo.used; ++i) {
      serializer_model_data_t* model = target->model_repo.data + i;
      node_t* source = scene->node_repo.nodes + i;

      model->transform = source->transform;

      // this will discard anything but 128 characters.
      memset(model->name.data, 0, sizeof(model->name.data));
      memcpy(model->name.data, source->name.data, sizeof(source->name.data));

      {
        uint32_t elem_count = sizeof(model->models.indices)/sizeof(uint32_t);
        // this only fits 1024 indices.
        model->meshes.used = source->meshes.count;
        if (model->meshes.used) {
          assert(model->meshes.used <= elem_count);
          memcpy(
            model->meshes.indices,
            source->meshes.indices,
            sizeof(uint32_t) * model->meshes.used);
        }

        // this only fits 1024 indices.
        model->models.used = source->nodes.count;
        if (model->models.used) {
          assert(model->models.used <= elem_count);
          memcpy(
            model->models.indices,
            source->nodes.indices,
            sizeof(uint32_t) * model->models.used);
        } 
      }
    }
  }
}

// NOTE(khalil): scene const?
static
void
copy_node_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  assert(scene->model_repo.used && "at least a root node must exist!");

  {
    target->node_repo.count = scene->model_repo.used; 
    target->node_repo.nodes = 
      allocate_node_array(scene->model_repo.used, allocator);
    
    for (uint32_t i = 0; i < target->node_repo.count; ++i) {
      node_t* node = target->node_repo.nodes + i;
      serializer_model_data_t* source = scene->model_repo.data + i;

      node->transform = source->transform;

      // this will discard anything but 128 characters.
      memset(node->name.data, 0, sizeof(node->name.data));
      memcpy(node->name.data, source->name.data, sizeof(node->name.data));

      node->meshes.count = source->meshes.used;
      if (node->meshes.count) {
        node->meshes.indices =
          allocator->mem_cont_alloc(node->meshes.count, sizeof(uint32_t));
        memcpy(
          node->meshes.indices,
          source->meshes.indices,
          sizeof(uint32_t) * node->meshes.count);
      }

      node->nodes.count = source->models.used;
      if (node->nodes.count) {
        node->nodes.indices =
          allocator->mem_cont_alloc(node->nodes.count, sizeof(uint32_t));
        memcpy(
          node->nodes.indices,
          source->models.indices,
          sizeof(uint32_t) * node->nodes.count);
      }
    }
  }
}

scene_t*
bin_to_scene(
  serializer_scene_data_t* scene,
  const allocator_t* allocator)
{
  assert(scene);
  assert(scene->model_repo.used != 0);
  
  {
    scene_t* runtime_scene = create_scene("current", allocator);
    copy_texture_data(scene, runtime_scene, allocator);
    copy_material_data(scene, runtime_scene, allocator);
    copy_mesh_data(scene, runtime_scene, allocator);
    copy_node_data(scene, runtime_scene, allocator);
    runtime_scene->font_repo.count = 0;
    runtime_scene->font_repo.fonts = NULL;
    return runtime_scene;
  }
}

serializer_scene_data_t*
scene_to_bin(
  scene_t* scene, 
  const allocator_t* allocator)
{
  assert(scene);
  assert(allocator);

  {
    serializer_scene_data_t* serializer_scene = 
      allocator->mem_alloc(sizeof(serializer_scene_data_t));
    populate_serializer_texture_data(scene, serializer_scene, allocator);
    populate_serializer_material_data(scene, serializer_scene, allocator);
    populate_serializer_mesh_data(scene, serializer_scene, allocator);
    populate_serializer_model_data(scene, serializer_scene, allocator);
    return serializer_scene;
  }
}