/**
 * @file map_to_mesh.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-02-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef MAP_TO_MESH_H
#define MAP_TO_MESH_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct allocator_t allocator_t;

// returns a mesh_t* typecast as void*.
void* 
map_to_mesh(void* map, const allocator_t* allocator);

// returns a serializer_scene_data_t* typecast as void*.
void*
map_to_bin(void* map, const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif