/**
 * @file serializer_bin.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <serializer/utils.h>
#include <serializer/serializer_bin.h>


void
serialize_bin(
  const char* path,
  serializer_scene_data_t* scene)
{  
  file_handle_t file;
  assert(scene != NULL && "'scene' params are NULL!");

  file = open_file(path, FILE_OPEN_MODE_WRITE | FILE_OPEN_MODE_BINARY);
  assert((void *)file != NULL);

  // serialize texture data.
  write_buffer(file, &scene->texture_repo.used, sizeof(uint32_t), 1);
  {
    serializer_texture_data_t *data = scene->texture_repo.data;
    for (uint32_t i = 0; i < scene->texture_repo.used; ++i, ++data) {
      write_buffer(
        file,
        data->path.data,
        sizeof(data->path.data[0]), sizeof(data->path.data));
    }
  }
  
  // serialize material data.
  write_buffer(file, &scene->material_repo.used, sizeof(uint32_t), 1);
  {
    serializer_material_data_t* data = scene->material_repo.data;
    for (uint32_t i = 0; i < scene->material_repo.used; ++i, ++data) {
      write_buffer(
        file, 
        data->name.data, 
        sizeof(data->name.data[0]), sizeof(data->name.data));

      write_buffer(
        file, data->ambient.data, sizeof(data->ambient.data[0]), 4);
      write_buffer(
        file, data->diffuse.data, sizeof(data->diffuse.data[0]), 4);
      write_buffer(
        file, data->specular.data, sizeof(data->specular.data[0]), 4);
      write_buffer(file, &data->shininess, sizeof(float), 1);
      write_buffer(file, &data->opacity, sizeof(float), 1);

      // serialize the texture properties array.
      write_buffer(file, &data->textures.used, sizeof(uint32_t), 1);
      {
        serializer_texture_properties_t* texture_data = data->textures.data;
        for (uint32_t j = 0; j < data->textures.used; ++j, ++texture_data) {
          // index into the texture repo.
          write_buffer(file, &texture_data->index, sizeof(uint32_t), 1);
          write_buffer(
            file, 
            texture_data->name.data,
            sizeof(texture_data->name.data[0]), 
            sizeof(texture_data->name.data));
          write_buffer(
            file, 
            texture_data->type.data,
            sizeof(texture_data->type.data[0]), 
            sizeof(texture_data->type.data));
          write_buffer(file, &texture_data->u, sizeof(float), 1);
          write_buffer(file, &texture_data->v, sizeof(float), 1);
          write_buffer(file, &texture_data->u_scale, sizeof(float), 1);
          write_buffer(file, &texture_data->v_scale, sizeof(float), 1);
          write_buffer(file, &texture_data->angle, sizeof(float), 1);
        }
      }
    }
  }

  // serialize the mesh data.
  write_buffer(file, &scene->mesh_repo.used, sizeof(uint32_t), 1);
  {
    serializer_mesh_data_t* data = scene->mesh_repo.data;
    for (uint32_t i = 0; i < scene->mesh_repo.used; ++i, ++data) {
      write_buffer(
        file, 
        data->name.data, 
        sizeof(data->name.data[0]), sizeof(data->name.data));
      // serialize matrix transform.
      write_buffer(
        file, 
        data->local_transform.data, 
        sizeof(data->local_transform.data[0]), 16);
      // serialize vertices data.
      write_buffer(file, &data->vertices_count, sizeof(uint32_t), 1);
      write_buffer(
        file, data->vertices, sizeof(float), data->vertices_count * 3);
      write_buffer(
        file, data->normals, sizeof(float), data->vertices_count * 3);
      write_buffer(
        file, data->uvs, sizeof(float), data->vertices_count * 3);
      // serialize faces data.
      write_buffer(file, &data->faces_count, sizeof(uint32_t), 1);
      write_buffer(file, data->indices, sizeof(uint32_t), data->faces_count * 3);

      // serializing material indices.
      write_buffer(file, &data->materials.used, sizeof(uint32_t), 1);
      write_buffer(
        file, 
        data->materials.indices, 
        sizeof(uint32_t), (size_t)data->materials.used);
    }
  }

  // serialize model data.
  write_buffer(file, &scene->model_repo.used, sizeof(uint32_t), 1);
  {
    serializer_model_data_t* data = scene->model_repo.data;
    for (uint32_t i = 0; i < scene->model_repo.used; ++i, ++data) {
      write_buffer(
        file, 
        data->name.data, 
        sizeof(data->name.data[0]), sizeof(data->name.data));
      write_buffer(
        file, 
        data->transform.data, 
        sizeof(data->transform.data[0]), 16);

      // write meshes indices, and models indices;
      write_buffer(file, &data->meshes.used, sizeof(uint32_t), 1);
      write_buffer(
        file, data->meshes.indices, sizeof(uint32_t), data->meshes.used);
      write_buffer(file, &data->models.used, sizeof(uint32_t), 1);
      write_buffer(
        file, data->models.indices, sizeof(uint32_t), data->models.used);
    }
  }

  close_file(file);
}

serializer_scene_data_t*
deserialize_bin(
  const char* path,
  const allocator_t* allocator)
{
  file_handle_t file;
  assert(allocator != NULL && "allocator is NULL!");

  file = open_file(path, FILE_OPEN_MODE_READ | FILE_OPEN_MODE_BINARY);
  assert((void *)file != NULL);

  {
    serializer_scene_data_t* scene = NULL;
    scene = (serializer_scene_data_t*)allocator->mem_alloc(
      sizeof(serializer_scene_data_t));
    assert(scene != NULL && "failed to allocate scene");

    // deserialize texture data.
    read_buffer(file, &scene->texture_repo.used, sizeof(uint32_t), 1);
    scene->texture_repo.data = 
      (serializer_texture_data_t *)allocator->mem_cont_alloc(
        scene->texture_repo.used, 
        sizeof(serializer_texture_data_t));
    {
      serializer_texture_data_t* data = scene->texture_repo.data;
      for (uint32_t i = 0; i < scene->texture_repo.used; ++i, ++data) {
        read_buffer(
          file,
          data->path.data,
          sizeof(data->path.data[0]), sizeof(data->path.data));
      }
    }

    // deserialize material data.
    read_buffer(file, &scene->material_repo.used, sizeof(uint32_t), 1);
    scene->material_repo.data = 
      (serializer_material_data_t *)allocator->mem_cont_alloc(
        scene->material_repo.used, 
        sizeof(serializer_material_data_t));
    {
      serializer_material_data_t* data = scene->material_repo.data;
      for (uint32_t i = 0; i < scene->material_repo.used; ++i, ++data) {
        read_buffer(
          file, 
          data->name.data, 
          sizeof(data->name.data[0]), sizeof(data->name.data));

        read_buffer(
          file, data->ambient.data, sizeof(data->ambient.data[0]), 4);
        read_buffer(
          file, data->diffuse.data, sizeof(data->diffuse.data[0]), 4);
        read_buffer(
          file, data->specular.data, sizeof(data->specular.data[0]), 4);
        read_buffer(file, &data->shininess, sizeof(float), 1);
        read_buffer(file, &data->opacity, sizeof(float), 1);

        // deserialize the texture properties array.
        read_buffer(file, &data->textures.used, sizeof(uint32_t), 1);
        {
          serializer_texture_properties_t* texture_data = data->textures.data;
          for (uint32_t j = 0; j < data->textures.used; ++j, ++texture_data) {
            // index into the texture repo.
            read_buffer(file, &texture_data->index, sizeof(uint32_t), 1);
            read_buffer(
              file, 
              texture_data->name.data,
              sizeof(texture_data->name.data[0]), 
              sizeof(texture_data->name.data));
            read_buffer(
              file, 
              texture_data->type.data,
              sizeof(texture_data->type.data[0]), 
              sizeof(texture_data->type.data));
            read_buffer(file, &texture_data->u, sizeof(float), 1);
            read_buffer(file, &texture_data->v, sizeof(float), 1);
            read_buffer(file, &texture_data->u_scale, sizeof(float), 1);
            read_buffer(file, &texture_data->v_scale, sizeof(float), 1);
            read_buffer(file, &texture_data->angle, sizeof(float), 1);
          }
        }
      }
    }

    // deserialize the mesh data.
    read_buffer(file, &scene->mesh_repo.used, sizeof(uint32_t), 1);
    scene->mesh_repo.data = 
      (serializer_mesh_data_t*)allocator->mem_cont_alloc(
        scene->mesh_repo.used, 
        sizeof(serializer_mesh_data_t));
    {
      serializer_mesh_data_t* data = scene->mesh_repo.data;
      for (uint32_t i = 0; i < scene->mesh_repo.used; ++i, ++data) {
        read_buffer(
          file, 
          data->name.data, 
          sizeof(data->name.data[0]), sizeof(data->name.data));
        // deserialize matrix transform.
        read_buffer(
          file, 
          data->local_transform.data, 
          sizeof(data->local_transform.data[0]), 16);
        // deserialize vertices data.
        read_buffer(file, &data->vertices_count, sizeof(uint32_t), 1);
        data->vertices = 
          (float*)allocator->mem_cont_alloc(
            data->vertices_count * 3, sizeof(float));
        data->normals = 
          (float*)allocator->mem_cont_alloc(
            data->vertices_count * 3, sizeof(float));
        data->uvs = 
          (float*)allocator->mem_cont_alloc(
            data->vertices_count * 3, sizeof(float));
        read_buffer(
          file, data->vertices, sizeof(float), data->vertices_count * 3);
        read_buffer(
          file, data->normals, sizeof(float), data->vertices_count * 3);
        read_buffer(
          file, data->uvs, sizeof(float), data->vertices_count * 3);
        // deserialize faces data.
        read_buffer(file, &data->faces_count, sizeof(uint32_t), 1);
        data->indices = 
          (uint32_t*)allocator->mem_cont_alloc(
            data->faces_count * 3, sizeof(uint32_t));
        read_buffer(file, data->indices, sizeof(uint32_t), data->faces_count * 3);

        // deserialize material indices.
        read_buffer(file, &data->materials.used, sizeof(uint32_t), 1);
        read_buffer(
          file, 
          data->materials.indices, 
          sizeof(uint32_t), (size_t)data->materials.used);
      }
    }

    // deserialize model data.
    read_buffer(file, &scene->model_repo.used, sizeof(uint32_t), 1);
    scene->model_repo.data = 
      (serializer_model_data_t*)allocator->mem_cont_alloc(
        scene->model_repo.used, 
        sizeof(serializer_model_data_t));
    {
      serializer_model_data_t* data = scene->model_repo.data;
      for (uint32_t i = 0; i < scene->model_repo.used; ++i, ++data) {
        read_buffer(
          file, 
          data->name.data, 
          sizeof(data->name.data[0]), sizeof(data->name.data));
        read_buffer(
          file, 
          data->transform.data, 
          sizeof(data->transform.data[0]), 16);

        // write meshes indices, and models indices;
        read_buffer(file, &data->meshes.used, sizeof(uint32_t), 1);
        read_buffer(
          file, data->meshes.indices, sizeof(uint32_t), data->meshes.used);
        read_buffer(file, &data->models.used, sizeof(uint32_t), 1);
        read_buffer(
          file, data->models.indices, sizeof(uint32_t), data->models.used);
      }
    }

    close_file(file);
    return scene;
  }
}

void
free_bin(
  serializer_scene_data_t* scene,
  const allocator_t* allocator)
{
  assert(scene != NULL && "scene is NULL!");
  assert(allocator != NULL && "allocator is NULL!");

  allocator->mem_free(scene->texture_repo.data);
  allocator->mem_free(scene->material_repo.data);
  {
    serializer_mesh_data_t* data = scene->mesh_repo.data;
    for (uint32_t i = 0; i < scene->mesh_repo.used; ++i, ++data) {
      allocator->mem_free(data->vertices);
      allocator->mem_free(data->normals);
      allocator->mem_free(data->uvs);
      allocator->mem_free(data->indices);
    }
    allocator->mem_free(scene->mesh_repo.data);
  }
  allocator->mem_free(scene->model_repo.data);
  allocator->mem_free(scene);
}