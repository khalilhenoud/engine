/**
 * @file data_format.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MESH_FORMAT_H
#define MESH_FORMAT_H


typedef
struct mesh_t {
  float* vertices;
  float* normals;
  float* uvs;
  uint32_t vertices_count;
  uint32_t* indices;
  uint32_t indices_count;
} mesh_t;

#endif