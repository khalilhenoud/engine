/**
 * @file bin_to_entity.cpp
 * @author khalilhenoud@gmail.com
 * @brief convert a bin scene into an entity scene.
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <cassert>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <entity/node.h>
#include <application/converters/bin_to_entity.h>
#include <serializer/serializer_scene_data.h>


std::unique_ptr<entity::node>
bin_to_scene(serializer_scene_data_t* scene)
{
  assert(scene);
  assert(scene->model_repo.used != 0);

  std::function<void(serializer_model_data_t*, entity::node*)> 
  copy_model_data = [&](serializer_model_data_t* source, entity::node* target) {
    target->m_name = source->name.data;
    // source->transform; currently the transform isn't used.

    // copy the meshes.
    for (uint32_t j = 0; j < source->meshes.used; ++j) {
      uint32_t mesh_index = source->meshes.indices[j];
      assert(mesh_index < scene->mesh_repo.used);

      serializer_mesh_data_t* s_mesh = scene->mesh_repo.data + mesh_index;
      // TODO: We need a mesh manager, to avoid duplicates and the likes.
      // TODO: Same for a texture manager, to avoid duplication and the likes.
      // That way we do not instantiate unique pointers if the meshes exists, 
      // rather we used shared pointers and the likes.
      target->m_meshes.emplace_back(std::make_shared<entity::mesh>());
      entity::mesh* entity_mesh = target->m_meshes.back().get();
      // s_mesh->local_transform; current the transform isn't used.
      entity_mesh->m_name = s_mesh->name.data;
      entity_mesh->m_vertices.insert(
        entity_mesh->m_vertices.end(), 
        s_mesh->vertices,
        s_mesh->vertices + s_mesh->vertices_count * 3);
      entity_mesh->m_normals.insert(
        entity_mesh->m_normals.end(), 
        s_mesh->normals,
        s_mesh->normals + s_mesh->vertices_count * 3);
      entity_mesh->m_uvCoords.push_back(std::vector<float>());
      entity_mesh->m_uvCoords[0].insert(
        entity_mesh->m_uvCoords[0].end(),
        s_mesh->uvs,
        s_mesh->uvs + s_mesh->vertices_count * 3);
      entity_mesh->m_indices.insert(
        entity_mesh->m_indices.end(),
        s_mesh->indices,
        s_mesh->indices + s_mesh->faces_count * 3);

      for (uint32_t k = 0; k < s_mesh->materials.used; ++k) {
        uint32_t material_index = s_mesh->materials.indices[k];
        assert(material_index < scene->material_repo.used);

        serializer_material_data_t* s_material = 
          scene->material_repo.data + material_index;

        entity_mesh->m_materials.emplace_back();
        entity::material& entity_material = entity_mesh->m_materials.back();
        entity_material.m_name = s_material->name.data;
        memcpy(
          &entity_material.m_ambient[0], 
          s_material->ambient.data, 
          sizeof(s_material->ambient.data));
        memcpy(
          &entity_material.m_diffuse[0], 
          s_material->diffuse.data, 
          sizeof(s_material->diffuse.data));
        memcpy(
          &entity_material.m_specular[0], 
          s_material->specular.data, 
          sizeof(s_material->specular.data));
        entity_material.m_shininess = s_material->shininess;
        entity_material.m_opacity = s_material->opacity;

        // now copy the textures.
        for (uint32_t l = 0; l < s_material->textures.used; ++l) {
          serializer_texture_properties_t* s_texture = 
            s_material->textures.data + l;
          entity_material.m_textures.emplace_back();
          entity::texture& entity_texture = entity_material.m_textures.back();
          entity_texture.m_angle = s_texture->angle;
          entity_texture.m_name = s_texture->name.data;
          entity_texture.m_type = s_texture->type.data;
          entity_texture.m_u = s_texture->u;
          entity_texture.m_u_scale = s_texture->u_scale;
          entity_texture.m_v = s_texture->v;
          entity_texture.m_v_scale = s_texture->v_scale;
          assert(s_texture->index < scene->texture_repo.used);
          entity_texture.m_path = 
            scene->texture_repo.data[s_texture->index].path.data;
        }
      }
    }

    // copy the rest of the models.
    for (uint32_t i = 0; i < source->models.used; ++i) {
      assert(source->models.indices[i] < scene->model_repo.used);

      serializer_model_data_t* child_source = 
        scene->model_repo.data + source->models.indices[i];
      target->m_children.push_back(std::make_shared<entity::node>());
      entity::node* child_current = target->m_children.back().get();
      copy_model_data(child_source, child_current);
    }
  };
  
  // root corresponding to the first model.
  std::unique_ptr<entity::node> root = std::make_unique<entity::node>();
  entity::node* entity_current = root.get();
  serializer_model_data_t* s_current = scene->model_repo.data + 0;
  copy_model_data(s_current, entity_current);

  return root;
}