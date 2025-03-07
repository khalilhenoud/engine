/**
 * @file application.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <algorithm>
#include <vector>

#include <library/allocator/allocator.h>
#include <renderer/renderer_opengl.h>
#include <application/application.h>

#include <entity/c/level/level.h>
#include <application/process/levels/generic_level.h>
#include <application/process/levels/room_select.h>


////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
extern "C" {
char to_load[256];

void
set_level_to_load(const char* source)
{
  memset(to_load, 0, sizeof(to_load));
  memcpy(to_load, source, strlen(source));
}
}

////////////////////////////////////////////////////////////////////////////////
level_t level;
allocator_t allocator;

static uint32_t in_level_select;
static int32_t viewport_width;
static int32_t viewport_height;
static const char* data_set;

static
void
cleanup_level()
{
  level.unload(&allocator);
  renderer_cleanup();
  assert(allocated.size() == 0 && "Memory leak detected!");
  in_level_select = !in_level_select;
}

static
level_context_t get_context()
{
  level_context_t context;
  context.data_set = data_set;
  context.viewport.x = context.viewport.y = 0;
  context.viewport.width = viewport_width;
  context.viewport.height = viewport_height;
  context.level = in_level_select ? "room_select" : to_load;
  return context;
}

// TODO: This is why we need a factory pattern construction system.
static
void
construct_level()
{
  if (in_level_select)
    construct_level_selector(&level);
  else
    construct_generic_level(&level);
} 

application::application(
  int32_t width,
  int32_t height,
  const char* dataset)
{
  renderer_initialize();

  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = reallocate;

  viewport_width = width;
  viewport_height = height;
  data_set = dataset;
  in_level_select = 1;

  construct_level();
  level.load(get_context(), &allocator);
}

application::~application()
{
  cleanup_level();
}

void 
application::update()
{
  level.update(&allocator);

  if (level.should_unload()) {
    cleanup_level();
    construct_level();
    level.load(get_context(), &allocator);
  }
}