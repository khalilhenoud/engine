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


////////////////////////////////////////////////////////////////////////////////
std::vector<uintptr_t> allocated;

void* allocate(size_t size)
{
  void* block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
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
pipeline_t pipeline;

// the scene and its renderer data.
scene_t* scene;
packaged_scene_render_data_t* scene_render_data;

// The font in question.
font_runtime_t* font;
uint32_t font_image_id;

camera_t* camera;
allocator_t allocator;

bvh_t* bvh;

application::application(
  int32_t width,
  int32_t height,
  const char* dataset)
  : m_dataset(dataset)
{
  renderer_initialize();

  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = nullptr;

  scene = load_scene_from_bin(
    dataset, 
    "map_textured2.bin", 
    1, 
    { 0.2f, 0.2f, 0.2f, 1.f}, 
    &allocator);

  // load the scene render data.
  scene_render_data = load_scene_render_data(scene, &allocator);
  prep_packaged_render_data(m_dataset.c_str(), scene_render_data, &allocator);

  // guaranteed to exist, same with the font.
  camera = scene_render_data->camera_data.cameras;

  // need to load the images required by the scene.
  font = scene_render_data->font_data.fonts;
  font_image_id = scene_render_data->font_data.texture_ids[0];

  bvh = create_bvh_from_scene(scene, &allocator);

  pipeline_set_default(&pipeline);
  set_viewport(&pipeline, 0.f, 0.f, float(width), float(height));
  update_viewport(&pipeline);

  /// "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
  float znear = 0.1f, zfar = 4000.f, aspect = (float)width / height;
  float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
  float fw = fh * aspect;
  set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
  update_projection(&pipeline);

  controller.lock_framerate(60);

  show_cursor(0);
}

application::~application()
{
  free_scene(scene, &allocator);
  cleanup_packaged_render_data(scene_render_data, &allocator);
  renderer_cleanup();
  free_bvh(bvh, &allocator);

  assert(allocated.size() == 0 && "Memory leak detected!");
}

uint64_t 
application::update()
{
  uint64_t frame_rate = controller.end();
  float dt_seconds = controller.start();

  input_update();
  clear_color_and_depth_buffers();

  render_packaged_scene_data(scene_render_data, &pipeline, camera);

  {
    // disable/enable input with '~' key.
    if (::is_key_triggered(VK_OEM_3)) {
      m_disable_input = !m_disable_input;
      ::show_cursor((int32_t)m_disable_input);

      if (m_disable_input)
        recenter_camera_cursor();
    }

    if (!m_disable_input) {
      camera_update(
        dt_seconds, 
        camera, 
        bvh,
        &pipeline, 
        font, 
        font_image_id);
    }
  }

  {
    char delta_str[128] = { 0 };
    sprintf(delta_str, "delta: %f", dt_seconds);
    char frame_str[128] = { 0 };
    sprintf(frame_str, "fps: %llu", frame_rate);

    color_t white = { 1.f, 1.f, 1.f, 1.f };
    // display simple instructions.
    std::vector<const char*> text;
    text.push_back(delta_str);
    text.push_back(frame_str);
    text.push_back("----------------");
    text.push_back("[C] RESET CAMERA");
    text.push_back("[~] CAMERA UNLOCK/LOCK");
    text.push_back("[1/2/WASD/EQ] CAMERA SPEED/MOVEMENT");
    render_text_to_screen(
      font, 
      font_image_id, 
      &pipeline, 
      &text[0], 
      (uint32_t)text.size(),
      &white, 
      0.f, 0.f);
  }

  ::flush_operations();
  
  return frame_rate;
}