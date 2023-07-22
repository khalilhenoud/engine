/**
 * @file bin_to_entity.h
 * @author khalilhenoud@gmail.com
 * @brief convert a bin scene into an entity scene.
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef BIN_TO_ENTITY_H
#define BIN_TO_ENTITY_H

#include <memory>


typedef struct serializer_scene_data_t serializer_scene_data_t;

namespace entity {
  struct node;
}

std::unique_ptr<entity::node>
bin_to_scene(serializer_scene_data_t* scene);

#endif