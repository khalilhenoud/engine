/**
 * @file ase_to_entity.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <application/converters/ase_to_entity.h>
#include <loaders/loader_ase.h>
#include <entity/node.h>
#include <cassert>
#include <functional>


std::unique_ptr<entity::node> 
load_ase_model(
  std::string& dataset, 
  std::string& path, 
  const allocator_t* allocator)
{
  auto fullpath = dataset + path;
  loader_ase_data_t* ase_data = load_ase(fullpath.c_str(), allocator);
  std::unique_ptr<entity::node> entity_model = std::make_unique<entity::node>();

  auto copy_textures = 
  [&](loader_texture_data_t& source, entity::texture& target) {
      target.m_name = source.name.data;
      target.m_path = source.path.data;
      target.m_type = source.type.data;
      target.m_u = source.u;
      target.m_v = source.v;
      target.m_u_scale = source.u_scale;
      target.m_v_scale = source.v_scale;
      target.m_angle = source.angle;
  };

  auto copy_materials = 
  [&](
    loader_ase_data_t* source, 
    uint32_t material_index, 
    entity::material& target) {
    assert(material_index < source->material_repo.used);
    auto material_data = source->material_repo.data[material_index];
    target.m_name = material_data.name.data;
    target.m_ambient = { 
      material_data.ambient.data[0], 
      material_data.ambient.data[1], 
      material_data.ambient.data[2], 
      material_data.ambient.data[3] };
    target.m_diffuse = { 
      material_data.diffuse.data[0], 
      material_data.diffuse.data[1], 
      material_data.diffuse.data[2], 
      material_data.diffuse.data[3] };
    target.m_specular = { 
      material_data.specular.data[0], 
      material_data.specular.data[1], 
      material_data.specular.data[2], 
      material_data.specular.data[3] };
    target.m_opacity = material_data.opacity;
    target.m_shininess = material_data.shininess;
    for (uint32_t i = 0; i < material_data.textures.used; ++i) {
      target.m_textures.push_back(entity::texture());
      copy_textures(material_data.textures.data[i], target.m_textures.back());
    }
  };

  auto copy_meshes = 
  [&](loader_ase_data_t* source, uint32_t mesh_index, entity::node* target) {
    assert(mesh_index < source->mesh_repo.used);
    auto mesh_data = source->mesh_repo.data[mesh_index];
    auto target_mesh = target->m_meshes.emplace_back(
      std::make_shared<entity::mesh>()).get();
    target_mesh->m_name = mesh_data.name.data;
    target_mesh->m_vertices.insert(
      target_mesh->m_vertices.end(), 
      &mesh_data.vertices[0],
      &mesh_data.vertices[mesh_data.vertices_count * 3]);
    target_mesh->m_normals.insert(
      target_mesh->m_normals.end(), 
      &mesh_data.normals[0],
      &mesh_data.normals[mesh_data.vertices_count * 3]);
    target_mesh->m_uvCoords.emplace_back(std::vector<float>());
    target_mesh->m_uvCoords[0].insert(
      target_mesh->m_uvCoords[0].end(), 
      &mesh_data.uvs[0],
      &mesh_data.uvs[mesh_data.vertices_count * 3]);
    target_mesh->m_indices.insert(
      target_mesh->m_indices.end(), 
      &mesh_data.indices[0],
      &mesh_data.indices[mesh_data.faces_count * 3]);
    for (uint32_t i = 0; i < mesh_data.materials.used; ++i) {
      uint32_t material_id = mesh_data.materials.indices[i];
      assert(material_id < source->material_repo.used);
      target_mesh->m_materials.push_back(entity::material());
      copy_materials(source, material_id, target_mesh->m_materials.back());
    }
  };

  using ftype = std::function<void(
    loader_ase_data_t* source, 
    uint32_t model_index, 
    entity::node*)>;
  ftype copy_models = 
  [&](loader_ase_data_t* source, uint32_t model_index, entity::node* target) {
    // this always starts with model_index = 0
    assert(model_index < source->model_repo.used);
    auto model_data = source->model_repo.data[model_index];
    
    target->m_name = model_data.name.data;

    for (uint32_t i = 0; i < model_data.meshes.used; ++i) {
      uint32_t mesh_index = model_data.meshes.indices[i];
      assert(mesh_index < source->mesh_repo.used);
      copy_meshes(
        source, 
        mesh_index, 
        target);
    }

    for (uint32_t i = 0; i < model_data.models.used; ++i) {
      uint32_t model_index = model_data.models.indices[i];
      assert(model_index < source->model_repo.used);
      copy_models(
        source, 
        model_index, 
        target->m_children.emplace_back(
          std::make_shared<entity::node>()).get());
    }
  };

  for (uint32_t i = 0; i < ase_data->model_repo.used; ++i) {
    copy_models(
      ase_data, 
      i, 
      entity_model->m_children.emplace_back(
        std::make_shared<entity::node>()).get());
  }
  
  free_ase(ase_data, allocator);
  return entity_model;
}