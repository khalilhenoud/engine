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
#include <math/c/vector3f.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <collision/capsule.h>
#include <collision/segment.h>
#include <collision/face.h>
#include <collision/sphere.h>
#include <serializer/serializer_bin.h>


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
static
void
prep_packaged_render_data(
  const char* data_set,
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator)
{
  // start with the mesh_data
  for (uint32_t i = 0; i < render_data->mesh_data.count; ++i) {
    texture_runtime_t* runtime = render_data->mesh_data.texture_runtimes + i;
    if (strlen(runtime->texture.path.data)) {
      load_image_buffer(data_set, runtime, allocator);
      render_data->mesh_data.texture_ids[i] = ::upload_to_gpu(
        runtime->texture.path.data,
        runtime->buffer,
        runtime->width,
        runtime->height,
        (renderer_image_format_t)runtime->format);
    }
  }

  // then do the font_data.
  for (uint32_t i = 0; i < render_data->font_data.count; ++i) {
    load_font_inplace(
      data_set, 
      &render_data->font_data.fonts[i].font,
      &render_data->font_data.fonts[i], 
      allocator);
    // TODO: Ultimately we need a system for this, we need to be able to handle
    // deduplication.
    texture_runtime_t* runtime = render_data->font_data.texture_runtimes + i;
    if (strlen(runtime->texture.path.data)) {
      load_image_buffer(data_set, runtime, allocator);
      render_data->font_data.texture_ids[i] = ::upload_to_gpu(
        runtime->texture.path.data,
        runtime->buffer,
        runtime->width,
        runtime->height,
        (renderer_image_format_t)runtime->format);
    }
  }
}

static
void
cleanup_packaged_render_data(
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator)
{
  for (uint32_t i = 0; i < render_data->mesh_data.count; ++i) {
    if (render_data->mesh_data.texture_ids[i])
      ::evict_from_gpu(render_data->mesh_data.texture_ids[i]);
  }

  for (uint32_t i = 0; i < render_data->font_data.count; ++i) {
    if (render_data->font_data.texture_ids[i])
      ::evict_from_gpu(render_data->font_data.texture_ids[i]);
  }

  free_render_data(render_data, allocator);
}

////////////////////////////////////////////////////////////////////////////////

framerate_controller controller;
pipeline_t pipeline;

// the scene and its renderer data.
scene_t* scene;
packaged_scene_render_data_t* scene_render_data;

// the capsule and its renderer data.
capsule_t capsule;
mesh_t* capsule_mesh;
packaged_mesh_data_t* capsule_render_data;

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
  ::renderer_initialize();

  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = nullptr;

  {
    // load the scene from a bin file.
    auto fullpath = m_dataset + "media\\cooked\\map2.bin";
    serializer_scene_data_t *scene_bin = ::deserialize_bin(
      fullpath.c_str(), &allocator);
    scene_bin->material_repo.data[0].ambient.data[0] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[1] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[2] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[3] = 1.f;
    scene = bin_to_scene(scene_bin, &allocator);
    ::free_bin(scene_bin, &allocator);
  }

  // load the scene render data.
  scene_render_data = load_scene_render_data(scene, &allocator);
  prep_packaged_render_data(m_dataset.c_str(), scene_render_data, &allocator);

  // guaranteed to exist, same with the font.
  camera = scene_render_data->camera_data.cameras;

  {
    // complex room.
    camera->position.data[0] = 1636;
    camera->position.data[1] = 149;
    camera->position.data[2] = 1781;
  }

  //{
  //  // long corridor.
  //  camera->position.data[0] = 889.193054;
  //  camera->position.data[1] = -25.5755959;
  //  camera->position.data[2] = -1536.21448;
  //}

  // need to load the images required by the scene.
  font = scene_render_data->font_data.fonts;
  font_image_id = scene_render_data->font_data.texture_ids[0];

  {
    // load the capsule mesh.
    memset(capsule.center.data, 0, sizeof(capsule.center.data));
    capsule.half_height = 30;
    capsule.radius = 25;
    capsule_mesh = create_unit_capsule(
      30, capsule.half_height / capsule.radius, &allocator);
    capsule_render_data = load_mesh_renderer_data(
      capsule_mesh, color_rgba_t{ 1.f, 1.f, 0.f, 0.5f }, &allocator);
  }

  {
    float** vertices = NULL;
    uint32_t** indices = NULL;
    uint32_t* indices_count = NULL;
    uint32_t mesh_count = 0;

    for (uint32_t i = 0; i < scene->mesh_repo.count; ++i) {
      if (scene->mesh_repo.meshes[i].indices_count)
        ++mesh_count;
    }

    if (mesh_count) {
      vertices = (float**)allocator.mem_alloc(sizeof(float*) * mesh_count);
      assert(vertices && "allocation failed!");
      indices = (uint32_t**)allocator.mem_alloc(sizeof(uint32_t*) * mesh_count);
      assert(indices && "allocation failed!");
      indices_count = (uint32_t*)allocator.mem_alloc(sizeof(uint32_t) * mesh_count); 
      assert(indices && "allocation failed!");

      for (uint32_t i = 0, k = 0; i < scene->mesh_repo.count; ++i) {
        if (scene->mesh_repo.meshes[i].indices_count) {
          mesh_t* mesh = scene->mesh_repo.meshes + i;
          vertices[k] = mesh->vertices;
          indices[k] = mesh->indices;
          indices_count[k] = mesh->indices_count;
          ++k;
        }
      }

      bvh = create_bvh(
        vertices, 
        indices, 
        indices_count, 
        mesh_count, 
        &allocator, 
        BVH_CONSTRUCT_NAIVE);

      allocator.mem_free(vertices);
      allocator.mem_free(indices);
      allocator.mem_free(indices_count);
    }
  }

  ::pipeline_set_default(&pipeline);
  ::set_viewport(&pipeline, 0.f, 0.f, float(width), float(height));
  ::update_viewport(&pipeline);

  /// "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
  float znear = 0.1f, zfar = 4000.f, aspect = (float)width / height;
  float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
  float fw = fh * aspect;
  ::set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
  ::update_projection(&pipeline);

  controller.lock_framerate(60);

  ::show_cursor(0);
}

application::~application()
{
  free_scene(scene, &allocator);
  cleanup_packaged_render_data(scene_render_data, &allocator);
  free_mesh(capsule_mesh, &allocator);
  free_mesh_render_data(capsule_render_data, &allocator);
  renderer_cleanup();
  free_bvh(bvh, &allocator);

  assert(allocated.size() == 0 && "Memory leak detected!");
}

uint64_t 
application::update()
{
  uint64_t frame_rate = controller.end();
  float dt_seconds = controller.start();

  ::input_update();
  ::clear_color_and_depth_buffers();

  matrix4f out;
  memset(&out, 0, sizeof(matrix4f));
  ::get_view_transformation(camera, &out);
  ::set_matrix_mode(&pipeline, MODELVIEW);
  ::load_identity(&pipeline);
  ::post_multiply(&pipeline, &out);

#if SHOW_GRID
  // draw grid.
  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, -100, 0);
    ::draw_grid(&pipeline, 5000.f, 100);
    ::pop_matrix(&pipeline);
  }
#endif

  if (scene) {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, 0, 0);
    ::pre_scale(&pipeline, 1, 1, 1);
    ::draw_meshes(
      scene_render_data->mesh_data.mesh_render_data, 
      scene_render_data->mesh_data.texture_ids, 
      scene_render_data->mesh_data.count, 
      &pipeline);
    ::pop_matrix(&pipeline);
  }

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#if 0
  static vector3f debug_position = { 1826.99866, -113.391373, -675.791565 };
  //static vector3f debug_position = { 1862.78345, 204.111115, 2034.12927 };
  //static vector3f debug_position = { 1569.43994, -90.2638550, 130.261658 };

  {
    capsule.center.data[0] = debug_position.data[0];
    capsule.center.data[1] = debug_position.data[1];
    capsule.center.data[2] = debug_position.data[2];

    {
      segment_t segment;
      get_capsule_segment(&capsule, &segment);

      float vertices[12];
      vertices[0 * 3 + 0] = segment.points[0].data[0];
      vertices[0 * 3 + 1] = segment.points[0].data[1];
      vertices[0 * 3 + 2] = segment.points[0].data[2];
      vertices[1 * 3 + 0] = segment.points[1].data[0];
      vertices[1 * 3 + 1] = segment.points[1].data[1];
      vertices[1 * 3 + 2] = segment.points[1].data[2];
      ::draw_lines(vertices, 2, { 0.f, 1.f, 1.f, 1.f }, 1, &pipeline);
    }

    {
      uint32_t capsule_texture_render_id[1] = { 0 };
      ::push_matrix(&pipeline);
      ::pre_translate(&pipeline, capsule.center.data[0], capsule.center.data[1], capsule.center.data[2]);
      float scale = (capsule.half_height + capsule.radius);
      ::pre_scale(&pipeline, scale, scale, scale);
      ::draw_meshes(capsule_render_data->mesh_render_data, capsule_texture_render_id, 1, &pipeline);
      ::pop_matrix(&pipeline);
    }
  }

  if (m_disable_input) {
    if (::is_key_pressed('W'))
      debug_position.data[0] += 2.f;
    if (::is_key_pressed('S'))
      debug_position.data[0] -= 2.f;
    if (::is_key_pressed('Q'))
      debug_position.data[1] += 2.f;
    if (::is_key_pressed('E'))
      debug_position.data[1] -= 2.f;
    if (::is_key_pressed('D'))
      debug_position.data[2] += 2.f;
    if (::is_key_pressed('A'))
      debug_position.data[2] -= 2.f;
  }
#endif
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

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
        &capsule, 
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
    text.push_back("[3/4] RENDER COLLISION QUERIES");
    text.push_back("[5] SHOW SNAPPING/FALLING STATE");
    text.push_back("[6] SHOW SNAPPING FACE");
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