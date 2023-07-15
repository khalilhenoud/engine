/**
 * @file entity_to_bin_data.h
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


struct allocator_t;
struct serializer_scene_data_t;

namespace entity {
  struct node;
}

serializer_scene_data_t*
convert_to_bin_format(
  entity::node& root, 
  const allocator_t* allocator);

#endif