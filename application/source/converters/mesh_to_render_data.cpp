/**
 * @file mesh_to_render_data.cpp
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <application/converters/mesh_to_render_data.h>
#include <data_format/mesh_format/data_format.h>


mesh_render_data_t
load_mesh_renderer_data(mesh_t* mesh, color_t color)
{
  mesh_render_data_t render_mesh = {
    &mesh->vertices[0], 
    &mesh->normals[0], 
    &mesh->uvs[0],
    mesh->vertices_count,
    &mesh->indices[0],
    mesh->indices_count,
    color,
    color,
    color
  };

  return render_mesh;
}