/**
 * @file room_select.c
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
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/level/level.h>
#include <application/process/text/utils.h>
#include <application/process/render_data/utils.h>
#include <application/converters/to_render_data.h>
#include <application/converters/bin_to_scene_to_bin.h>
#include <application/converters/to_render_data.h>
#include <library/filesystem/filesystem.h>
#include <library/framerate_controller/framerate_controller.h>

#define KEY_EXIT_LEVEL           '0'


static framerate_controller_t controller;
static int32_t exit_room_select = -1;
static color_t white = { 1.f, 1.f, 1.f, 1.f };
static color_rgba_t scene_color = { 0.2f, 0.2f, 0.2f, 1.f};
static pipeline_t pipeline;
static scene_t* scene;
static packaged_scene_render_data_t* scene_render_data;
static font_runtime_t* font;
static uint32_t font_image_id;
static dir_entries_t rooms;

static
void
load_rooms(const char* dataset)
{
  char directory[260];
  sprintf(directory, "%srooms\\*", dataset);
  get_subdirectories(directory, &rooms);
}

void
load_room_select(
  const level_context_t context,
  const allocator_t* allocator)
{
  char room[256] = {0};
  sprintf(room, "rooms\\%s", context.level);

  scene = load_scene_from_bin(
    context.data_set,
    room,
    context.level,
    0, 
    scene_color, 
    allocator);

  // load the scene render data.
  scene_render_data = load_scene_render_data(scene, allocator);
  prep_packaged_render_data(
    context.data_set, room, scene_render_data, allocator);

  // need to load the images required by the scene.
  font = scene_render_data->font_data.fonts;
  font_image_id = scene_render_data->font_data.texture_ids[0];

  load_rooms(context.data_set);
  exit_room_select = -1;

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
}

void
update_room_select(const allocator_t* allocator)
{
  uint64_t frame_rate = (uint64_t)controller_end(&controller);
  float dt_seconds = (float)controller_start(&controller);

  input_update();
  clear_color_and_depth_buffers();

  {
    char rooms_names[1024][260];
    const char* text[1024];
    uint32_t used = rooms.used;

    for (uint32_t i = 0; i < rooms.used; ++i) {
      if (strcmp(rooms.dir_names[i], "room_select") == 0)
        sprintf(rooms_names[i], "[%s] %s", "-", rooms.dir_names[i]);
      else
        sprintf(rooms_names[i], "[%u] %s", (i + 1), rooms.dir_names[i]);
      text[i] = rooms_names[i];
    }

    render_text_to_screen(
      font, 
      font_image_id,
      &pipeline, 
      text, 
      used,
      &white, 
      20.f, 0.f);
  }

  // only support 9 levels for now.
  for (int32_t i = 0; i < 9; ++i) {
    if (
      is_key_triggered('1' + i) && 
      i < (int32_t)rooms.used && 
      strcmp(rooms.dir_names[i], "room_select") != 0) {
        exit_room_select = i;
        break;
      } 
  }

  flush_operations();
}

void
unload_room_select(const allocator_t* allocator)
{
  free_scene(scene, allocator);
  cleanup_packaged_render_data(scene_render_data, allocator);
}

void
set_level_to_load(const char* source);

uint32_t
should_unload_room_select(void)
{
  if (exit_room_select == -1)
    return 0;
  else {
    set_level_to_load(rooms.dir_names[exit_room_select]);
    return 1;
  }
}

void
construct_level_selector(level_t* level)
{
  assert(level);

  level->load = load_room_select;
  level->update = update_room_select;
  level->unload = unload_room_select;
  level->should_unload = should_unload_room_select;
}