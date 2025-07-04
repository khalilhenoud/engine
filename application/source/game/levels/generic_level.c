/**
 * @file level1.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <assert.h>
#include <application/input.h>
#include <application/game/debug/flags.h>
#include <application/game/logic/player.h>
#include <application/game/levels/load_scene.h>
#include <application/game/rendering/render_data.h>
#include <application/game/debug/text.h>
#include <library/framerate_controller/framerate_controller.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/scene.h>
#include <entity/c/level/level.h>
#include <entity/c/spatial/bvh.h>

#define DEBUG_MESHES 0
#if DEBUG_MESHES
#include <stdlib.h>
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/scene/node.h>
#endif

#define TILDE   0xC0
#define KEY_EXIT_LEVEL           '0'


static framerate_controller_t controller;
static uint32_t exit_level = 0;
static int32_t disable_input;
static pipeline_t pipeline;
static camera_t* camera;
static scene_t* scene;
static packaged_scene_render_data_t* scene_render_data;
static font_runtime_t* font;
static uint32_t font_image_id;
static bvh_t* bvh;

void
load_level(
  const level_context_t context,
  const allocator_t* allocator)
{
  char room[256] = {0};
  sprintf(room, "rooms\\%s", context.level);
  scene = load_scene(context.data_set, room, context.level, allocator);

#if DEBUG_MESHES
  {
    mesh_t *unit_capsule = create_unit_capsule(32, 12.f/16.f, allocator);
    uint32_t index = scene->mesh_repo.count;
    ++scene->mesh_repo.count;
    scene->mesh_repo.meshes = (mesh_t *)allocator->mem_realloc(
      scene->mesh_repo.meshes, sizeof(mesh_t) * scene->mesh_repo.count);
    assert(scene->mesh_repo.meshes);
    mesh_t *curr = scene->mesh_repo.meshes + index;
    *curr = *unit_capsule;

    // cleanup the previously created mesh
    unit_capsule->indices = NULL;
    unit_capsule->indices_count = 0;
    unit_capsule->materials.used = 0;
    unit_capsule->normals = NULL;
    unit_capsule->uvs = NULL;
    unit_capsule->vertices = NULL;
    unit_capsule->vertices_count = 0;
    allocator->mem_free(unit_capsule);

    uint32_t nindex = scene->node_repo.count;
    ++scene->node_repo.count;
    scene->node_repo.nodes = (node_t *)allocator->mem_realloc(
      scene->node_repo.nodes, sizeof(node_t) * scene->node_repo.count);
    assert(scene->node_repo.nodes);
    node_t *node = scene->node_repo.nodes + nindex;
    node->meshes.count = 1;
    node->meshes.indices = allocator->mem_alloc(sizeof(uint32_t));
    node->meshes.indices[0] = index;
    // transform
    matrix4f m1, m2, m3, tmp;
    matrix4f_scale(&m1, 32.f, 32.f, 32.f);
    matrix4f_translation(&m2, 565.480103f, 44.0000153f, -684.309448f);
    // scene->node_repo.nodes[0]
    m3 = inverse_m4f(&scene->node_repo.nodes[0].transform);
    tmp = mult_m4f(&m2, &m1);
    node->transform = mult_m4f(&m3, &tmp);
    // important
    node->nodes.count = 0;
    node->name = cstring_create("tmp1", allocator);
    
    // set to use the scene.
    uint32_t idx = scene->node_repo.nodes[0].nodes.count; 
    scene->node_repo.nodes[0].nodes.count++;
    if (idx) {
      scene->node_repo.nodes[0].nodes.indices = 
        (uint32_t *)allocator->mem_realloc(
          scene->node_repo.nodes[0].nodes.indices, 
          sizeof(uint32_t) * scene->node_repo.nodes[0].nodes.count);
    } else {
      scene->node_repo.nodes[0].nodes.indices = 
      (uint32_t *)allocator->mem_alloc(
        sizeof(uint32_t) * scene->node_repo.nodes[0].nodes.count);
    }
    scene->node_repo.nodes[0].nodes.indices[idx] = nindex;

    /* VERIFY
    This does not collides.
    {565.480103, 44.0000153, -684.309448}

    This collides
    {559.823242, 44.0000153, -689.966309}
    */
  }
#endif
  
  scene_render_data = load_scene_render_data(scene, allocator);
  prep_packaged_render_data(
    context.data_set, room, scene_render_data, allocator);

  // guaranteed to exist, same with the font.
  camera = scene_render_data->camera_data.cameras;

  font = scene_render_data->font_data.fonts;
  font_image_id = scene_render_data->font_data.texture_ids[0];

  bvh = scene->bvh_repo.bvhs + 0;

  exit_level = 0;
  disable_input = 0;

  pipeline_set_default(&pipeline);
  set_viewport(
    &pipeline, 0.f, 0.f, 
    (float)context.viewport.width, (float)context.viewport.height);
  update_viewport(&pipeline);

  {
    // "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
    float znear = 0.1f, zfar = 4000.f;
    float aspect = (float)context.viewport.width / context.viewport.height;
    float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
    float fw = fh * aspect;
    set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
    update_projection(&pipeline);
  }

  show_cursor(0);

  initialize_controller(&controller, 60, 1u);

  player_init(
    scene->metadata.player_start, 
    scene->metadata.player_angle, 
    camera, 
    bvh);
}

void
update_level(const allocator_t* allocator)
{
  uint64_t frame_rate = (uint64_t)controller_end(&controller);
  float dt_seconds = (float)controller_start(&controller);

  input_update();
  clear_color_and_depth_buffers();

  render_packaged_scene_data(scene_render_data, &pipeline, camera);

  {
    // disable/enable input with '~' key.
    if (is_key_triggered(TILDE)) {
      disable_input = !disable_input;
      show_cursor((int32_t)disable_input);
    }

    if (!disable_input) {
      update_debug_flags();
      player_update(dt_seconds);
      draw_debug_text_frame(&pipeline, font, font_image_id);
      draw_debug_face_frame(&pipeline, g_debug_flags.disable_depth_debug);
    } else {
      if (is_key_triggered(KEY_EXIT_LEVEL))
        exit_level = 1;
    }
  }

  if (disable_input) {
    const char* text[1];

    text[0] = "press [0] to return to room selection";
    render_text_to_screen(
      font, 
      font_image_id, 
      &pipeline, 
      text, 
      1,
      white, 
      0.f, 0.f);
  } else {
    const char* text[6];
    char delta_str[128] = { 0 };
    char frame_str[128] = { 0 };

    sprintf(delta_str, "delta: %f", dt_seconds);
    sprintf(frame_str, "fps: %llu", frame_rate);

    // display simple instructions.
    text[0] = delta_str;
    text[1] = frame_str;
    text[2] = "----------------";
    text[3] = "[C] RESET CAMERA";
    text[4] = "[~] CAMERA UNLOCK/LOCK";
    text[5] = "[1/2/WASD/EQ] CAMERA SPEED/MOVEMENT";
    render_text_to_screen(
      font, 
      font_image_id, 
      &pipeline, 
      text, 
      6,
      white, 
      0.f, 0.f);
  }

  flush_operations();
}

void
unload_level(const allocator_t* allocator)
{
  scene_free(scene, allocator);
  cleanup_packaged_render_data(scene_render_data, allocator);
}

uint32_t
should_unload(void)
{
  return exit_level;
}

void
construct_generic_level(level_t* level)
{
  assert(level);

  level->load = load_level;
  level->update = update_level;
  level->unload = unload_level;
  level->should_unload = should_unload;
}
