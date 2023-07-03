/**
 * @file entity_to_render_data.h
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef ENTITY_TO_RENDER_DATA
#define ENTITY_TO_RENDER_DATA

#include <vector>
#include <utility>


struct mesh_render_data_t;

namespace entity {
  struct node;
}

std::pair<std::vector<mesh_render_data_t>, std::vector<const char*>>
load_renderer_data(entity::node& model);

#endif