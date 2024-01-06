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
#include <application/input.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/scene.h>
#include <entity/c/scene/scene_utils.h>
#include <application/process/text/utils.h>
#include <application/process/logic/camera.h>
#include <application/process/spatial/bvh.h>
#include <application/process/spatial/bvh_utils.h>
#include <application/process/render_data/utils.h>
#include <application/converters/to_render_data.h>
#include <application/converters/bin_to_scene_to_bin.h>
#include <application/converters/to_render_data.h>

#define TILDE   0xC0


color_t white = { 1.f, 1.f, 1.f, 1.f };
color_rgba_t scene_color = { 0.2f, 0.2f, 0.2f, 1.f};
int32_t disable_input;
pipeline_t pipeline;
camera_t* camera;
scene_t* scene;
packaged_scene_render_data_t* scene_render_data;
font_runtime_t* font;
uint32_t font_image_id;
bvh_t* bvh;

void
load_level(
  const char* data_set, 
  const char* file, 
  float width,
  float height,
  const allocator_t* allocator)
{
  char room[256] = {0};
  sprintf(room, "rooms\\%s", file);

  scene = load_scene_from_bin(
    data_set,
    room,
    file,
    1, 
    scene_color, 
    allocator);

  // load the scene render data.
  scene_render_data = load_scene_render_data(scene, allocator);
  prep_packaged_render_data(data_set, room, scene_render_data, allocator);

  // guaranteed to exist, same with the font.
  camera = scene_render_data->camera_data.cameras;

  // need to load the images required by the scene.
  font = scene_render_data->font_data.fonts;
  font_image_id = scene_render_data->font_data.texture_ids[0];

  bvh = create_bvh_from_scene(scene, allocator);

  pipeline_set_default(&pipeline);
  set_viewport(&pipeline, 0.f, 0.f, width, height);
  update_viewport(&pipeline);

  {
    // "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
    float znear = 0.1f, zfar = 4000.f, aspect = (float)width / height;
    float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
    float fw = fh * aspect;
    set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
    update_projection(&pipeline);
  }

  show_cursor(0);
}

void
update_level(
  float dt_seconds, 
  uint64_t framerate, 
  const allocator_t* allocator)
{
  input_update();
  clear_color_and_depth_buffers();

  render_packaged_scene_data(scene_render_data, &pipeline, camera);

  {
    // disable/enable input with '~' key.
    if (is_key_triggered(TILDE)) {
      disable_input = !disable_input;
      show_cursor((int32_t)disable_input);

      if (disable_input)
        recenter_camera_cursor();
    }

    if (!disable_input) {
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
    const char* text[6];
    char delta_str[128] = { 0 };
    char frame_str[128] = { 0 };

    sprintf(delta_str, "delta: %f", dt_seconds);
    sprintf(frame_str, "fps: %llu", framerate);

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
      &white, 
      0.f, 0.f);
  }

  flush_operations();
}

void
unload_level(const allocator_t* allocator)
{
  free_scene(scene, allocator);
  cleanup_packaged_render_data(scene_render_data, allocator);
  free_bvh(bvh, allocator);
}