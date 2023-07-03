/**
 * @file ase_to_entity.h
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef ASE_TO_ENTITY
#define ASE_TO_ENTITY

#include <memory>
#include <string>


typedef struct allocator_t allocator_t;

namespace entity {
  struct node;
}

std::unique_ptr<entity::node> 
load_ase_model(
  std::string& dataset, 
  std::string& path, 
  const allocator_t* allocator);

#endif