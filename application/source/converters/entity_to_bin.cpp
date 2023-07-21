/**
 * @file entity_to_bin.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>
#include <entity/node.h>
#include <library/allocator/allocator.h>
#include <serializer/serializer_scene_data.h>


serializer_scene_data_t*
convert_to_bin_format(
  entity::node& root, 
  const allocator_t* allocator)
{
  serializer_scene_data_t* scene = 
    (serializer_scene_data_t*)allocator->mem_alloc(
      sizeof(serializer_scene_data_t));

  std::unordered_map<std::string, uint32_t> texture_to_index;
  std::vector<std::string> textures = root.get_textures_paths();
  std::vector<entity::mesh *> meshes = root.get_meshes();
  std::vector<entity::node *> nodes = root.get_nodes();

  // convert the textures.
  scene->texture_repo.used = (uint32_t)textures.size();
  scene->texture_repo.data = 
    (serializer_texture_data_t *)allocator->mem_cont_alloc(
      scene->texture_repo.used, 
      sizeof(serializer_texture_data_t));

  for (uint32_t i = 0; i < scene->texture_repo.used; ++i) {
    memset(
      scene->texture_repo.data[i].path.data, 0, 
      sizeof(scene->texture_repo.data[i].path.data));
    memcpy(
      scene->texture_repo.data[i].path.data, 
      textures[i].c_str(), 
      textures[i].length());

    // keep a dictionary to these.
    texture_to_index[textures[i]] = i;
  }

  // find the total number of materials, we need to allocate them.
  size_t total_material_count = 0, material_index = 0, mesh_index = 0;
  for (auto mesh : meshes)
    total_material_count += mesh->m_materials.size();

  scene->material_repo.used = (uint32_t)total_material_count;
  scene->material_repo.data = 
    (serializer_material_data_t *)allocator->mem_cont_alloc(
      scene->material_repo.used, 
      sizeof(serializer_material_data_t));

  scene->mesh_repo.used = (uint32_t)meshes.size();
  scene->mesh_repo.data = 
    (serializer_mesh_data_t *)allocator->mem_cont_alloc(
      scene->mesh_repo.used, 
      sizeof(serializer_mesh_data_t));
  
  for (auto mesh : meshes) {
    {
      serializer_mesh_data_t* mesh_data = &scene->mesh_repo.data[mesh_index++];
      ::matrix4f_set_identity(&mesh_data->local_transform);
      mesh_data->faces_count = (uint32_t)mesh->m_indices.size() / 3;
      mesh_data->vertices_count = (uint32_t)mesh->m_vertices.size() / 3;
      memset(mesh_data->name.data, 0, sizeof(mesh_data->name.data));
      memcpy(mesh_data->name.data, mesh->m_name.c_str(), mesh->m_name.length());
      
      size_t total_alloc_vertices = sizeof(float) * mesh->m_vertices.size();
      mesh_data->vertices = (float*)allocator->mem_alloc(total_alloc_vertices);
      memcpy(mesh_data->vertices, mesh->m_vertices.data(), total_alloc_vertices);
      mesh_data->uvs = (float*)allocator->mem_alloc(total_alloc_vertices);
      memcpy(mesh_data->uvs, mesh->m_uvCoords[0].data(), total_alloc_vertices);
      mesh_data->normals = (float*)allocator->mem_alloc(total_alloc_vertices);
      memcpy(mesh_data->normals, mesh->m_normals.data(), total_alloc_vertices);

      size_t total_alloc_indices = sizeof(uint32_t) * mesh->m_indices.size();
      mesh_data->indices = (uint32_t*)allocator->mem_alloc(total_alloc_indices);
      memcpy(mesh_data->indices, mesh->m_indices.data(), total_alloc_indices);

      mesh_data->materials.used = (uint32_t)mesh->m_materials.size();
      for (uint32_t i = 0; i < mesh_data->materials.used; ++i)
        mesh_data->materials.indices[i] = material_index + i;
    }

    for (auto material : mesh->m_materials) {
      auto& current = scene->material_repo.data[material_index++];
      memset(current.name.data, 0, sizeof(current.name.data));
      memcpy(current.name.data,
        material.m_name.c_str(),
        material.m_name.length());

      memcpy(current.ambient.data, &material.m_ambient[0], 4);
      memcpy(current.diffuse.data, &material.m_diffuse[0], 4);
      memcpy(current.specular.data, &material.m_specular[0], 4);
      current.opacity = material.m_opacity;
      current.shininess = material.m_shininess;

      // copy over the texture details.
      size_t max_size = 
        (sizeof(current.textures.data)/sizeof(current.textures.data[0]));
      current.textures.used = material.m_textures.size();
      current.textures.used = 
        max_size < current.textures.used ? max_size : current.textures.used;
      
      uint32_t texture_index = 0;
      for (auto& texture_data : material.m_textures) {
        if (texture_index > current.textures.used)
          break;
        auto& texture_prop = current.textures.data[texture_index++];
        texture_prop.angle = texture_data.m_angle;
        texture_prop.u = texture_data.m_u;
        texture_prop.v = texture_data.m_v;
        texture_prop.u_scale = texture_data.m_u_scale;
        texture_prop.v_scale = texture_data.m_v_scale;
        memcpy(
          texture_prop.name.data, 
          texture_data.m_name.c_str(), 
          texture_data.m_name.length());
        memcpy(
          texture_prop.type.data, 
          texture_data.m_type.c_str(), 
          texture_data.m_type.length());
        texture_prop.index = texture_to_index[texture_data.m_path];
      }
    }
  }

  // convert the nodes.
  nodes.insert(nodes.begin(), &root);
  scene->model_repo.used = (uint32_t)nodes.size();
  scene->model_repo.data = 
    (serializer_model_data_t *)allocator->mem_cont_alloc(
      scene->model_repo.used, 
      sizeof(serializer_model_data_t));

  size_t node_index = 0;
  uint32_t to_mesh = 0;
  uint32_t to_model = 1;
  for (auto node : nodes) {
    auto current_node = &scene->model_repo.data[node_index++];
    ::matrix4f_set_identity(&current_node->transform);
    memset(current_node->name.data, 0, sizeof(current_node->name.data));
    memcpy(
      current_node->name.data, 
      node->m_name.c_str(), node->m_name.length());
    
    current_node->meshes.used = (uint32_t)node->m_meshes.size();
    for (uint32_t i = 0; i < current_node->meshes.used; ++i)
      current_node->meshes.indices[i] = to_mesh++;

    current_node->models.used = (uint32_t)node->m_children.size();
    for (uint32_t i = 0; i < current_node->models.used; ++i)
      current_node->models.indices[i] = to_model++;
  }

  return scene;
}