/**
 * @file entity_to_render_data.cpp
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <application/converters/entity_to_render_data.h>
#include <renderer/renderer_opengl_data.h>
#include <entity/node.h>


std::pair<std::vector<mesh_render_data_t>, std::vector<const char*>>
load_renderer_data(entity::node& model)
{
  std::vector<mesh_render_data_t> render_meshes;
  std::vector<const char*> texture_paths;
  auto entity_meshes = model.get_meshes();

  for (auto& mesh : entity_meshes) {
    if (mesh->m_materials.size()) {
      auto& material = mesh->m_materials[0];
      mesh_render_data_t render_mesh = {
        &mesh->m_vertices[0], 
        &mesh->m_normals[0], 
        &(mesh->m_uvCoords[0])[0],
        (uint32_t)mesh->m_vertices.size()/3,
        &mesh->m_indices[0],
        (uint32_t)mesh->m_indices.size(),
        { 
          material.m_ambient[0], material.m_ambient[1], 
          material.m_ambient[2], material.m_ambient[3] 
        },
        { 
          material.m_diffuse[0], material.m_diffuse[1], 
          material.m_diffuse[2], material.m_diffuse[3] 
        },
        { 
          material.m_specular[0], material.m_specular[1], 
          material.m_specular[2], material.m_specular[3] 
        }
      };
      render_meshes.emplace_back(render_mesh);
      texture_paths.push_back(
        material.m_textures.size() ? 
        material.m_textures[0].m_path.c_str() : nullptr);
    } else {
      mesh_render_data_t render_mesh = {
        &mesh->m_vertices[0], 
        &mesh->m_normals[0], 
        &(mesh->m_uvCoords[0])[0],
        (uint32_t)mesh->m_vertices.size()/3,
        &mesh->m_indices[0],
        (uint32_t)mesh->m_indices.size(),
      };
      render_meshes.emplace_back(render_mesh);
      texture_paths.push_back(nullptr);
    }
  }

  return std::make_pair(render_meshes, texture_paths);
}