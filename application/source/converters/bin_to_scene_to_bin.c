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
#include <serializer/serializer_scene_data.h>
#include <serializer/serializer_bin.h>
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
          scene->texture_repo.textures[i].path->str, 
          scene->texture_repo.textures[i].path->size);
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
        memcpy(mat->name.data, source->name->str, sizeof(source->name->size));
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
      memcpy(model->name.data, source->name->str, sizeof(source->name->size));

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
populate_serializer_font_data(
  scene_t* scene, 
  serializer_scene_data_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);

  {
    target->font_repo.used = scene->font_repo.count;
    target->font_repo.data = NULL;
    if (scene->font_repo.count) {
      target->font_repo.data = 
        (serializer_font_t*)allocator->mem_cont_alloc(
          scene->font_repo.count, sizeof(serializer_font_t));

      for (uint32_t i = 0; i < scene->font_repo.count; ++i) {
        font_t* source = scene->font_repo.fonts + i;
        serializer_font_t* font = target->font_repo.data + i;

        memset(font->data_file.data, 0, sizeof(font->data_file.data));
        memcpy(
          font->data_file.data, 
          source->data_file->str, 
          source->data_file->size);

        memset(font->image_file.data, 0, sizeof(font->image_file.data));
        memcpy(
          font->image_file.data, 
          source->image_file->str, 
          source->image_file->size);
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
populate_serializer_camera_data(
  scene_t* scene, 
  serializer_scene_data_t* target, 
  const allocator_t* allocator)
{
  assert(scene && target && allocator);
  
  {
    target->camera_repo.used = scene->camera_repo.count;
    target->camera_repo.data = NULL;
    if (scene->camera_repo.count) {
      target->camera_repo.data = 
        (serializer_camera_t*)allocator->mem_cont_alloc(
          scene->camera_repo.count, sizeof(serializer_camera_t));

      for (uint32_t i = 0; i < scene->camera_repo.count; ++i) {
        camera_t* source = scene->camera_repo.cameras + i;
        serializer_camera_t* camera = target->camera_repo.data + i;

        camera->position = source->position;
        camera->lookat_direction = source->lookat_direction;
        camera->up_vector = source->up_vector;
      }
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
    copy_font_data(scene, runtime_scene, allocator);
    copy_camera_data(scene, runtime_scene, allocator);
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
    populate_serializer_font_data(scene, serializer_scene, allocator);
    populate_serializer_camera_data(scene, serializer_scene, allocator);
    return serializer_scene;
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