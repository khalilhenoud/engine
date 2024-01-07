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
#include <windows.h>
#include <cassert>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <vector>
#include <entity/c/runtime/texture.h>
#include <entity/c/runtime/texture_utils.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <application/application.h>
#include <application/framerate_controller.h>
#include <application/input.h>
#include <application/converters/to_render_data.h>
#include <application/converters/png_to_image.h>
#include <application/converters/csv_to_font.h>
#include <application/converters/bin_to_scene_to_bin.h>
#include <application/process/text/utils.h>
#include <application/process/logic/camera.h>
#include <application/process/spatial/bvh.h>
#include <application/process/spatial/bvh_utils.h>
#include <application/process/render_data/utils.h>
#include <math/c/vector3f.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <collision/capsule.h>
#include <collision/segment.h>
#include <collision/face.h>
#include <library/allocator/allocator.h>
#include <library/filesystem/filesystem.h>
#include <application/process/levels/level1.h>
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
framerate_controller controller;
allocator_t allocator;

static uint32_t in_level;
static int32_t iwidth;
static int32_t iheight;
static const char* data_set;

static
void
cleanup_level()
{
  if (in_level) {
    in_level = 0;

    unload_level(&allocator);
    renderer_cleanup();
    assert(allocated.size() == 0 && "Memory leak detected!");
  } else {
    in_level = 1;

    unload_room_select(&allocator);
    renderer_cleanup();
    assert(allocated.size() == 0 && "Memory leak detected!");
  }
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
  allocator.mem_realloc = nullptr;

  in_level = 0;
  iwidth = width;
  iheight = height;
  data_set = dataset;

  load_room_select(
    dataset, 
    "room_select", 
    (float)iwidth, 
    (float)iheight, 
    &allocator);

  controller.lock_framerate(60);
}

application::~application()
{
  cleanup_level();
}

uint64_t 
application::update()
{
  uint64_t frame_rate = controller.end();
  float dt_seconds = controller.start();

  if (in_level) {
    update_level(dt_seconds, frame_rate, &allocator);
    
    if (should_unload()) {
      cleanup_level();

      load_room_select(
        data_set, 
        "room_select", 
        (float)iwidth, 
        (float)iheight, 
        &allocator);
    }
  } else {
    update_room_select(dt_seconds, frame_rate, &allocator);
    
    if (should_unload_room_select() != NULL) {
      const char* source = should_unload_room_select();
      char to_load[256];
      memset(to_load, 0, sizeof(to_load));
      memcpy(to_load, source, strlen(source));
      cleanup_level();

      in_level = 1;
      load_level(
        data_set, 
        to_load, 
        (float)iwidth, 
        (float)iheight, 
        &allocator);
    }
  }
  
  return frame_rate;
}