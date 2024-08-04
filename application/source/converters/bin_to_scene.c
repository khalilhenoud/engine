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
#include <library/string/string.h>
#include <entity/c/scene/node.h>
#include <entity/c/scene/node_utils.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/mesh/texture.h>
#include <entity/c/mesh/texture_utils.h>
#include <entity/c/mesh/material.h>
#include <entity/c/mesh/material_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/misc/font.h>
#include <entity/c/misc/font_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <entity/c/scene/light.h>
#include <entity/c/scene/light_utils.h>
#include <entity/c/spatial/bvh.h>
#include <entity/c/spatial/bvh_utils.h>
#include <serializer/serializer_scene_data.h>
#include <serializer/serializer_bin.h>
#include <application/converters/bin_to_scene.h>


static
void
copy_metadata(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->metadata.player_start = scene->metadata.player_start;
    target->metadata.player_angle = scene->metadata.player_angle;
  }
}

static
void
copy_light_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->light_repo.count = scene->light_repo.used;
    if (target->light_repo.count) {
      target->light_repo.lights = 
        allocator->mem_cont_alloc(
          target->light_repo.count, 
          sizeof(serializer_light_data_t));

      for (uint32_t i = 0; i < target->light_repo.count; ++i) {
        light_t* t_light = target->light_repo.lights + i;
        serializer_light_data_t* s_light = scene->light_repo.data + i;
        t_light->name = allocate_string(s_light->name.data, allocator);
        memcpy(
          t_light->position.data, 
          s_light->position.data,
          sizeof(t_light->position.data));
        memcpy(
          t_light->direction.data, 
          s_light->direction.data,
          sizeof(t_light->direction.data));
        memcpy(
          t_light->up.data, 
          s_light->up.data,
          sizeof(t_light->up.data));
        memcpy(&t_light->inner_cone, &s_light->inner_cone, sizeof(float));
        memcpy(&t_light->outer_cone, &s_light->outer_cone, sizeof(float));
        memcpy(
          &t_light->attenuation_constant, 
          &s_light->attenuation_constant, 
          sizeof(float));
        memcpy(
          &t_light->attenuation_linear, 
          &s_light->attenuation_linear, 
          sizeof(float));
        memcpy(
          &t_light->attenuation_quadratic, 
          &s_light->attenuation_quadratic, 
          sizeof(float));
        memcpy(
          t_light->diffuse.data, 
          s_light->diffuse.data, 
          sizeof(t_light->diffuse.data));
        memcpy(
          t_light->specular.data, 
          s_light->specular.data, 
          sizeof(t_light->specular.data));
        memcpy(
          t_light->ambient.data, 
          s_light->ambient.data, 
          sizeof(t_light->ambient.data));
        memcpy(
          &t_light->type, 
          &s_light->type, 
          sizeof(t_light->type));
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
        allocate_texture_array(scene->texture_repo.used, allocator);

      for (uint32_t i = 0; i < scene->texture_repo.used; ++i) {
        target->texture_repo.textures[i].path = 
          allocate_string(scene->texture_repo.data[i].path.data, allocator);
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
        allocate_material_array(scene->material_repo.used, allocator);
      
      for (uint32_t i = 0; i < scene->material_repo.used; ++i) {
        material_t* mat = target->material_repo.materials + i;
        serializer_material_data_t* source = scene->material_repo.data + i;

        // will only keep 128 characters.
        mat->name = allocate_string(source->name.data, allocator);
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
copy_mesh_data(
  serializer_scene_data_t* scene,
  scene_t* target,
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  assert(scene->mesh_repo.used);

  {
    target->mesh_repo.count = scene->mesh_repo.used;
    target->mesh_repo.meshes = 
      allocate_mesh_array(scene->mesh_repo.used, allocator);

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
      node->name = allocate_string(source->name.data, allocator);
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

static
void
copy_default_font(font_t* font, const allocator_t* allocator)
{
  assert(font);

  {
    const char* default_data_file = "\\font\\FontData.csv";
    const char* default_image_file = "\\font\\ExportedFont.png";
    font->data_file = allocate_string(default_data_file, allocator);
    font->image_file = allocate_string(default_image_file, allocator);
  }
}

static
void
copy_font_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  
  {
    target->font_repo.count = scene->font_repo.used;
    target->font_repo.fonts = NULL;
    if (scene->font_repo.used) {
      target->font_repo.fonts = 
        allocate_font_array(scene->font_repo.used, allocator);

      for (uint32_t i = 0; i < scene->font_repo.used; ++i) {
        serializer_font_t* source = scene->font_repo.data + i;
        font_t* font = target->font_repo.fonts + i;

        font->data_file = allocate_string(source->data_file.data, allocator);
        font->image_file = allocate_string(source->image_file.data, allocator);
      }
    } else {
      // ensure at least the default font in the scene.
      target->font_repo.count = 1;
      target->font_repo.fonts = 
        allocate_font_array(target->font_repo.count, allocator);

      copy_default_font(target->font_repo.fonts, allocator);
    }
  }
}

static
void
copy_default_camera(camera_t* camera)
{
  vector3f center, at, up;
  center.data[0] = center.data[1] = center.data[2] = 0.f;
  at.data[0] = at.data[1] = at.data[2] = 0.f;
  at.data[2] = -1.f;
  up.data[0] = up.data[1] = up.data[2] = 0.f;
  up.data[1] = 1.f;
  set_camera_lookat(camera, center, at, up);
}

static
void
copy_camera_data(
  serializer_scene_data_t* scene, 
  scene_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  
  {
    target->camera_repo.count = scene->camera_repo.used;
    target->camera_repo.cameras = NULL;
    if (scene->camera_repo.used) {
      target->camera_repo.cameras = 
        (camera_t*)allocator->mem_cont_alloc(
          scene->camera_repo.used, sizeof(camera_t));

      for (uint32_t i = 0; i < scene->camera_repo.used; ++i) {
        serializer_camera_t* source = scene->camera_repo.data + i;
        camera_t* camera = target->camera_repo.cameras + i;

        camera->position = source->position;
        camera->lookat_direction = source->lookat_direction;
        camera->up_vector = source->up_vector;
      }
    } else {
      // ensure at least the default font in the scene.
      target->camera_repo.count = 1;
      target->camera_repo.cameras = 
        (camera_t*)allocator->mem_cont_alloc(
          target->camera_repo.count, sizeof(camera_t));

      copy_default_camera(target->camera_repo.cameras);
    }
  }
}

static
void
copy_bvh_data(
  serializer_scene_data_t* scene,
  scene_t* target,
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->bvh_repo.count = scene->bvh_repo.used;
    target->bvh_repo.bvhs = NULL;

    if (target->bvh_repo.count) { 
      target->bvh_repo.bvhs = allocate_bvh_array(
        scene->bvh_repo.used, allocator);

      for (uint32_t i = 0; i < target->bvh_repo.count; ++i) {
        bvh_t *bvh = target->bvh_repo.bvhs + i;
        serializer_bvh_t *source = scene->bvh_repo.data + i;

        // move the pointers around.
        bvh->count = source->count;
        bvh->faces = source->faces;
        source->faces = NULL;
        bvh->normals = source->normals;
        source->normals = NULL;
        bvh->bounds = (bvh_aabb_t*)source->bounds;
        source->bounds = NULL;

        bvh->nodes_used = source->nodes_used;
        bvh->nodes = (bvh_node_t*)source->nodes;
        source->nodes = NULL;
      }
    }
  }
}

static
scene_t*
bin_to_scene(
  serializer_scene_data_t* scene,
  const allocator_t* allocator)
{
  assert(scene);
  assert(scene->model_repo.used != 0);
  
  {
    scene_t* runtime_scene = create_scene("current", allocator);
    copy_metadata(scene, runtime_scene, allocator);
    copy_light_data(scene, runtime_scene, allocator);
    copy_texture_data(scene, runtime_scene, allocator);
    copy_material_data(scene, runtime_scene, allocator);
    copy_mesh_data(scene, runtime_scene, allocator);
    copy_node_data(scene, runtime_scene, allocator);
    copy_font_data(scene, runtime_scene, allocator);
    copy_camera_data(scene, runtime_scene, allocator);
    copy_bvh_data(scene, runtime_scene, allocator);
    return runtime_scene;
  }
}

scene_t*
load_scene_from_bin(
  const char* dataset, 
  const char* folder, 
  const char* file,
  uint32_t override_ambient, 
  color_rgba_t ambient, 
  const allocator_t* allocator)
{
  // TODO: This is temporary, this needs to be generalized in packaged content.
  char fullpath[1024] = {0};
  snprintf(fullpath, 1024, "%s\\%s\\%s.bin", dataset, folder, file);
  
  {
    scene_t* local = NULL;
    serializer_scene_data_t *scene_bin = deserialize_bin(fullpath, allocator);
    if (override_ambient) {
      scene_bin->material_repo.data[0].ambient.data[0] = ambient.data[0];
      scene_bin->material_repo.data[0].ambient.data[1] = ambient.data[1];
      scene_bin->material_repo.data[0].ambient.data[2] = ambient.data[2];
      scene_bin->material_repo.data[0].ambient.data[3] = ambient.data[3];
    }
    local = bin_to_scene(scene_bin, allocator);
    free_bin(scene_bin, allocator);
    return local;
  }
}