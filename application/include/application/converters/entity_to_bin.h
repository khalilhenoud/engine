/**
 * @file entity_to_bin.h
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between the entity and the serializer bin code.
 * @version 0.1
 * @date 2023-07-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef ENTITY_TO_BIN_DATA_H
#define ENTITY_TO_BIN_DATA_H


typedef struct allocator_t allocator_t;
typedef struct serializer_scene_data_t serializer_scene_data_t;

namespace entity {
  struct node;
}

serializer_scene_data_t*
scene_to_bin(
  entity::node& root, 
  const allocator_t* allocator);

#endif