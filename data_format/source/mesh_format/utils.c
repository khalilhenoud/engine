/**
 * @file utils.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <math/c/common.h>
#include <data_format/mesh_format/data_format.h>
#include <data_format/mesh_format/utils.h>


mesh_t*
create_unit_sphere(const int32_t factor, const allocator_t* allocator)
{
  const double half_pi = K_PI/2.;
  const double height_angle_increment = K_PI/(factor + 1);
  double ring_increment = half_pi/factor;
  uint32_t vertices_count = factor * factor * 4 + 2;
  uint32_t faces_count = factor * 4 * 2 + (factor - 1) * (factor * 4 * 2); 
  uint32_t indices_count = faces_count * 3;

  mesh_t* mesh = (mesh_t*)allocator->mem_alloc(sizeof(mesh_t));
  
  assert(mesh != NULL);
  assert(factor >= 1);

  {
    mesh->vertices = NULL;
    mesh->normals = NULL;
    mesh->uvs = NULL;
    mesh->indices = NULL;
  }

  mesh->vertices = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  mesh->vertices_count = vertices_count;

  mesh->normals = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  mesh->uvs = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  memset(mesh->uvs, 0, sizeof(float) * 3 * vertices_count);

  mesh->indices = (uint32_t*)allocator->mem_alloc(sizeof(uint32_t) * indices_count);
  mesh->indices_count = indices_count;

  // push the top and bottom vertices (0, 1, 0), (0, -1, 0) respectively.
  mesh->vertices[0 * 3 + 0] = 0.f;
  mesh->vertices[0 * 3 + 1] = 1.f;
  mesh->vertices[0 * 3 + 2] = 0.f;
  mesh->normals[0 * 3 + 0] = 0.f;
  mesh->normals[0 * 3 + 1] = 1.f;
  mesh->normals[0 * 3 + 2] = 0.f;

  mesh->vertices[(vertices_count - 1) * 3 + 0] = 0.f;
  mesh->vertices[(vertices_count - 1) * 3 + 1] = -1.f;
  mesh->vertices[(vertices_count - 1) * 3 + 2] = 0.f;
  mesh->normals[(vertices_count - 1) * 3 + 0] = 0.f;
  mesh->normals[(vertices_count - 1) * 3 + 1] = -1.f;
  mesh->normals[(vertices_count - 1) * 3 + 2] = 0.f;

  for (int32_t level = 1, current_vertex = 1; level <= factor; ++level) {
    double height_angle_total = height_angle_increment * level;
    double height = cos(height_angle_total);        // y
    double radius = sin(height_angle_total);

    for (int32_t ring_vertex = 0, total = factor * 4; ring_vertex < total; ++ring_vertex) {
      double ring_angle = ring_vertex * ring_increment;
      double horizontal_offset = cos(ring_angle) * radius;   // x
      double depth_offset = sin(ring_angle) * radius;        // z

      mesh->vertices[current_vertex * 3 + 0] = horizontal_offset;
      mesh->vertices[current_vertex * 3 + 1] = height;
      mesh->vertices[current_vertex * 3 + 2] = depth_offset;
      mesh->normals[current_vertex * 3 + 0] = horizontal_offset;
      mesh->normals[current_vertex * 3 + 1] = height;
      mesh->normals[current_vertex * 3 + 2] = depth_offset;
      ++current_vertex;
    }
  }

  // build the indices.
  {
    int32_t current_face = 0;
    int32_t starting_vertex = 1;
    int32_t faces_per_ring = factor * 4;
    // build the top ribbon indices.
    for (int32_t i = 0; i < faces_per_ring; ++starting_vertex, ++current_face, ++i) {
      mesh->indices[current_face * 3 + 2] = 0;
      mesh->indices[current_face * 3 + 1] = starting_vertex + 0;
      mesh->indices[current_face * 3 + 0] = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);
    }

    // build the intermediate ribbons indices.
    for (int32_t level = 1; level < factor; ++level) {
      for (int32_t i = 0, starting_vertex = 1 + (level - 1) * faces_per_ring; i < faces_per_ring; ++starting_vertex, ++i) {
        int32_t next_vertex = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);

        mesh->indices[current_face * 3 + 2] = starting_vertex;
        mesh->indices[current_face * 3 + 1] = starting_vertex + faces_per_ring;
        mesh->indices[current_face * 3 + 0] = next_vertex;
        ++current_face;

        mesh->indices[current_face * 3 + 2] = next_vertex;
        mesh->indices[current_face * 3 + 1] = starting_vertex + faces_per_ring;
        mesh->indices[current_face * 3 + 0] = next_vertex + faces_per_ring;
        ++current_face;
      }
    }

    // build the bottom ribbon indices.
    starting_vertex = vertices_count - faces_per_ring - 1;
    for (int32_t i = 0; i < faces_per_ring; ++starting_vertex, ++current_face, ++i) {
      mesh->indices[current_face * 3 + 2] = starting_vertex + 0;
      mesh->indices[current_face * 3 + 1] = vertices_count - 1;
      mesh->indices[current_face * 3 + 0] = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);
    }
  }

  return mesh;
}

mesh_t*
create_unit_capsule(
  const int32_t factor, 
  const float half_height_to_radius_ratio,
  const allocator_t* allocator)
{
  const double half_pi = K_PI / 2.;
  const double height_angle_increment = half_pi / factor;
  double ring_increment = half_pi / factor;
  uint32_t vertices_ring_count = factor * 4;
  uint32_t total_ring_count = factor * 2; // exluding the tip vertices.
  const uint32_t tip_vertices_count = 2;
  uint32_t vertices_count = total_ring_count * vertices_ring_count + tip_vertices_count;
  uint32_t fan_faces_count = factor * 4;
  uint32_t ribbon_face_count = factor * 4 * 2;
  uint32_t faces_count = fan_faces_count * 2 + (factor - 1) * 2 * ribbon_face_count + ribbon_face_count;
  uint32_t indices_count = faces_count * 3;
  float total_sections = half_height_to_radius_ratio + 1.f;
  float height_to_add = (1.f / total_sections) * half_height_to_radius_ratio;
  float radius_ratio = height_to_add * 1.f / half_height_to_radius_ratio;

  mesh_t* mesh = (mesh_t*)allocator->mem_alloc(sizeof(mesh_t));
  assert(mesh != NULL);
  assert(half_height_to_radius_ratio > 0.f);
  assert(factor >= 1);

  {
    mesh->vertices = NULL;
    mesh->normals = NULL;
    mesh->uvs = NULL;
    mesh->indices = NULL;
  }

  mesh->vertices = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  mesh->vertices_count = vertices_count;

  mesh->normals = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  mesh->uvs = (float*)allocator->mem_alloc(sizeof(float) * 3 * vertices_count);
  memset(mesh->uvs, 0, sizeof(float) * 3 * vertices_count);

  mesh->indices = (uint32_t*)allocator->mem_alloc(sizeof(uint32_t) * indices_count);
  mesh->indices_count = indices_count;

  // push the top and bottom vertices (0, 1, 0), (0, -1, 0) respectively.
  mesh->vertices[0 * 3 + 0] = 0.f;
  mesh->vertices[0 * 3 + 1] = 1.f;// + height_to_add;
  mesh->vertices[0 * 3 + 2] = 0.f;
  mesh->normals[0 * 3 + 0] = 0.f;
  mesh->normals[0 * 3 + 1] = 1.f;
  mesh->normals[0 * 3 + 2] = 0.f;

  mesh->vertices[(vertices_count - 1) * 3 + 0] = 0.f;
  mesh->vertices[(vertices_count - 1) * 3 + 1] = -1.f;// - height_to_add;
  mesh->vertices[(vertices_count - 1) * 3 + 2] = 0.f;
  mesh->normals[(vertices_count - 1) * 3 + 0] = 0.f;
  mesh->normals[(vertices_count - 1) * 3 + 1] = -1.f;
  mesh->normals[(vertices_count - 1) * 3 + 2] = 0.f;

  // create the vertices, in case of a mesh we do this in 2 halfs, we also
  // need to duplicate the middle ring's vertices.
  {
    int32_t current_vertex = 1;
    // top half of the mesh.
    for (int32_t level = 1; level <= factor; ++level) {
      double height_angle_total = height_angle_increment * level;
      double height = cos(height_angle_total) * radius_ratio;        // y
      double radius = sin(height_angle_total) * radius_ratio;

      for (int32_t ring_vertex = 0, total = factor * 4; ring_vertex < total; ++ring_vertex) {
        double ring_angle = ring_vertex * ring_increment;
        double horizontal_offset = cos(ring_angle) * radius;   // x
        double depth_offset = sin(ring_angle) * radius;        // z

        mesh->vertices[current_vertex * 3 + 0] = horizontal_offset;
        mesh->vertices[current_vertex * 3 + 1] = height + height_to_add;
        mesh->vertices[current_vertex * 3 + 2] = depth_offset;
        mesh->normals[current_vertex * 3 + 0] = horizontal_offset;
        mesh->normals[current_vertex * 3 + 1] = height;
        mesh->normals[current_vertex * 3 + 2] = depth_offset;
        ++current_vertex;
      }
    }

    // bottom half of the capule.
    for (int32_t level = 0; level < factor; ++level) {
      double height_angle_total = half_pi + height_angle_increment * level;
      double height = cos(height_angle_total) * radius_ratio;        // y
      double radius = sin(height_angle_total) * radius_ratio;

      for (int32_t ring_vertex = 0, total = factor * 4; ring_vertex < total; ++ring_vertex) {
        double ring_angle = ring_vertex * ring_increment;
        double horizontal_offset = cos(ring_angle) * radius;   // x
        double depth_offset = sin(ring_angle) * radius;        // z

        mesh->vertices[current_vertex * 3 + 0] = horizontal_offset;
        mesh->vertices[current_vertex * 3 + 1] = height - height_to_add;
        mesh->vertices[current_vertex * 3 + 2] = depth_offset;
        mesh->normals[current_vertex * 3 + 0] = horizontal_offset;
        mesh->normals[current_vertex * 3 + 1] = height;
        mesh->normals[current_vertex * 3 + 2] = depth_offset;
        ++current_vertex;
      }
    }
  }

  // build the indices.
  {
    int32_t current_face = 0;
    int32_t starting_vertex = 1;
    int32_t faces_per_ring = factor * 4;
    // build the top ribbon indices.
    for (int32_t i = 0; i < faces_per_ring; ++starting_vertex, ++current_face, ++i) {
      mesh->indices[current_face * 3 + 2] = 0;
      mesh->indices[current_face * 3 + 1] = starting_vertex + 0;
      mesh->indices[current_face * 3 + 0] = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);
    }

    // build the intermediate ribbons indices.
    for (int32_t level = 0, count = (factor - 1) * 2 + 1; level < count; ++level) {
      for (int32_t i = 0, starting_vertex = 1 + level * faces_per_ring; i < faces_per_ring; ++starting_vertex, ++i) {
        int32_t next_vertex = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);

        mesh->indices[current_face * 3 + 2] = starting_vertex;
        mesh->indices[current_face * 3 + 1] = starting_vertex + faces_per_ring;
        mesh->indices[current_face * 3 + 0] = next_vertex;
        ++current_face;

        mesh->indices[current_face * 3 + 2] = next_vertex;
        mesh->indices[current_face * 3 + 1] = starting_vertex + faces_per_ring;
        mesh->indices[current_face * 3 + 0] = next_vertex + faces_per_ring;
        ++current_face;
      }
    }

    // build the bottom ribbon indices.
    starting_vertex = vertices_count - faces_per_ring - 1;
    for (int32_t i = 0; i < faces_per_ring; ++starting_vertex, ++current_face, ++i) {
      mesh->indices[current_face * 3 + 2] = starting_vertex + 0;
      mesh->indices[current_face * 3 + 1] = vertices_count - 1;
      mesh->indices[current_face * 3 + 0] = starting_vertex + 1 - ((i == faces_per_ring - 1) ? faces_per_ring : 0);
    }
  }

  return mesh;
}

void
free_mesh(mesh_t* mesh, const allocator_t *allocator)
{
  allocator->mem_free(mesh->vertices);
  allocator->mem_free(mesh->normals);
  allocator->mem_free(mesh->uvs);
  allocator->mem_free(mesh->indices);
  allocator->mem_free(mesh);
}