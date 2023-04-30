/**
 * @file loader_ase_mesh_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef LOADER_ASE_MESH_DATA_H
#define LOADER_ASE_MESH_DATA_H

#include <stdint.h>
#include <library/string/fixed_string.h>


typedef
struct {
  fixed_str_t name;
  fixed_str_t path;
  fixed_str_t type;
  float u, v;
  float u_scale, v_scale;
  float angle;
} loader_texture_data_t;

typedef
struct {
  uint32_t used;
  loader_texture_data_t data[8];
} loader_texture_data_array_t;

typedef 
struct {
  float data[4];
} loader_color_data_t;

typedef 
struct {
  fixed_str_t name;
  loader_color_data_t ambient;
  loader_color_data_t diffuse;
  loader_color_data_t specular;
  float shininess;
  float opacity;
  loader_texture_data_array_t textures;
} loader_material_data_t;

typedef
struct {
  uint32_t used;
  uint32_t indices[1024];
} fixed_idx_to_target_t; 

typedef 
struct {
  fixed_str_t name;
  float *vertices;   // 3
  float *normals;    // 3
  float *uvs;        // 3 
  uint32_t vertices_count;
  uint32_t *indices;
  uint32_t faces_count;
  fixed_idx_to_target_t materials;
} loader_mesh_data_t;

typedef 
struct {
  fixed_str_t name;
  fixed_idx_to_target_t meshes;
  fixed_idx_to_target_t models;
} loader_model_data_t;

typedef 
struct {
  uint32_t used;
  loader_material_data_t *data;
} loader_material_repo_t;

typedef
struct {
  uint32_t used;
  loader_mesh_data_t *data;
} loader_mesh_repo_t;

typedef
struct {
  uint32_t used;
  loader_model_data_t *data;
} loader_model_repo_t;

typedef
struct {
  loader_model_repo_t model_repo;   // root = model_repo.data[0]
  loader_mesh_repo_t mesh_repo;
  loader_material_repo_t material_repo;
} loader_ase_data_t;

#endif