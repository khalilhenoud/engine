/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-04-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MESH_FORMAT_UTILS_H
#define MESH_FORMAT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <data_format/internal/module.h>
#include <library/allocator/allocator.h>


DATA_FORMAT_API
mesh_t*
create_unit_sphere(const int32_t factor, const allocator_t* allocator);

DATA_FORMAT_API
mesh_t*
create_unit_capsule(
  const int32_t factor, 
  const float half_height_to_radius_ratio,
  const allocator_t* allocator);

DATA_FORMAT_API
void
free_mesh(mesh_t* mesh, const allocator_t *allocator);

#ifdef __cplusplus
}
#endif

#endif