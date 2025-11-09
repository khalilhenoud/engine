/**
 * @file memory_tracking.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <application/game/memory_tracking/memory_tracking.h>


static
std::vector<uintptr_t> allocated;

void* allocate(size_t size)
{
  void* block = malloc(size);
  assert(block);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
  assert(block);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* reallocate(void* block, size_t size)
{
  void* tmp = realloc(block, size);
  assert(tmp);

  uintptr_t item = (uintptr_t)block;
  auto iter = std::find(allocated.begin(), allocated.end(), item);
  assert(iter != allocated.end());
  allocated.erase(iter);
  
  block = tmp;
  allocated.push_back(uintptr_t(block));

  return block;
}

void free_block(void* block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(), 
      allocated.end(), 
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }), 
    allocated.end());
  free(block);
}

void 
track_allocator_memory(allocator_t *allocator)
{
  allocator->mem_alloc = allocate;
  allocator->mem_cont_alloc = container_allocate;
  allocator->mem_free = free_block;
  allocator->mem_alloc_alligned = nullptr;
  allocator->mem_realloc = reallocate;
}

void
ensure_no_leaks(void)
{
  assert(allocated.size() == 0 && "Memory leak detected!");
}