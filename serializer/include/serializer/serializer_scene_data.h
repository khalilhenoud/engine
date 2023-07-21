/**
 * @file serializer_scene_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef SERIALIZER_MESH_DATA_H
#define SERIALIZER_MESH_DATA_H

#include <stdint.h>
#include <library/string/fixed_string.h>
#include <math/c/matrix4f.h>


typedef
struct {
  fixed_str_t path;
} serializer_texture_data_t;

typedef
struct {
  uint32_t index;               // index into texture repo.
  fixed_str_t name;
  fixed_str_t type;
  float u, v;       // TODO: Consider replacing this with a 3 x 3 matrix.
  float u_scale, v_scale;
  float angle;
} serializer_texture_properties_t;

// limit is 8 textures per material.
typedef
struct {
  uint32_t used;
  serializer_texture_properties_t data[8];
} serializer_texture_properties_array_t;

typedef 
struct {
  float data[4];
} serializer_color_data_t;

typedef 
struct {
  fixed_str_t name;
  serializer_color_data_t ambient;
  serializer_color_data_t diffuse;
  serializer_color_data_t specular;
  float shininess;
  float opacity;
  serializer_texture_properties_array_t textures;
} serializer_material_data_t;

typedef
struct {
  uint32_t used;
  uint32_t indices[1024];
} fixed_idx_to_target_t; 

typedef 
struct {
  fixed_str_t name;
  matrix4f local_transform;
  float *vertices;   // 3
  float *normals;    // 3
  float *uvs;        // 3 
  uint32_t vertices_count;
  uint32_t *indices;
  uint32_t faces_count;
  fixed_idx_to_target_t materials;
} serializer_mesh_data_t;

typedef 
struct {
  fixed_str_t name;
  matrix4f transform;
  fixed_idx_to_target_t meshes;
  fixed_idx_to_target_t models;
} serializer_model_data_t;

typedef
struct {
  uint32_t used;
  serializer_texture_data_t *data;
} serializer_texture_repo_t;

typedef 
struct {
  uint32_t used;
  serializer_material_data_t *data;
} serializer_material_repo_t;

typedef
struct {
  uint32_t used;
  serializer_mesh_data_t *data;
} serializer_mesh_repo_t;

typedef
struct {
  uint32_t used;
  serializer_model_data_t *data;
} serializer_model_repo_t;

typedef
struct serializer_scene_data_t {
  serializer_model_repo_t model_repo;   // root = model_repo.data[0]
  serializer_mesh_repo_t mesh_repo;
  serializer_material_repo_t material_repo;
  serializer_texture_repo_t texture_repo;
} serializer_scene_data_t;


#endif