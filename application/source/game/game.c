/**
 * @file game.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <windows.h>
#include <library/allocator/allocator.h>
#include <library/misc/precision_timers.h>
#include <application/game/game.h>
#include <application/game/levels/generic_level.h>
#include <application/game/levels/room_select.h>
#include <application/game/memory_tracking/memory_tracking.h>
#include <application/game/input/platform/input_platform.h>
#include <entity/c/level/level.h>
#include <renderer/renderer_opengl.h>
#include <renderer/platform/opengl_platform.h>
#include <windowing/windowing.h>


char to_load[256];

void
set_level_to_load(const char* source)
{
  memset(to_load, 0, sizeof(to_load));
  memcpy(to_load, source, strlen(source));
}

level_t level;
allocator_t allocator;
window_data_t window_data;

static uint32_t in_level_select;
static int32_t viewport_width;
static int32_t viewport_height;
static const char* data_set;

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

// TODO: This is why we need a factory pattern construction system
static
void
construct_level()
{
  if (in_level_select)
    construct_level_selector(&level);
  else
    construct_generic_level(&level);
} 

void 
game_init(
  int32_t width,
  int32_t height,
  const char *dataset)
{
  input_parameters_t input_params;
  opengl_parameters_t opengl_params;

  window_data = create_window(
    "custom_window", 
    "game", 
    width, 
    height);

  set_periodic_timers_resolution(1);
  track_allocator_memory(&allocator);

  input_params.window_handle = (HWND*)&window_data.handle;
  input_set_client(&input_params);

  opengl_params.device_context = (HDC*)&window_data.device_context;
  opengl_initialize(&opengl_params);
  renderer_initialize();

  viewport_width = width;
  viewport_height = height;
  data_set = dataset;
  in_level_select = 1;

  construct_level();
  level.load(get_context(), &allocator);
}

static
void
level_cleanup_internal()
{
  level.unload(&allocator);
  renderer_cleanup();
  ensure_no_leaks();
  in_level_select = !in_level_select;
}

void
game_cleanup()
{
  level_cleanup_internal();

  opengl_cleanup();
  destroy_window(&window_data);
  end_periodic_timers_resolution(1);
}

static
void
game_update_internal(void)
{
  level.update(&allocator);

  if (level.should_unload()) {
    level_cleanup_internal();
    construct_level();
    level.load(get_context(), &allocator);
  }

  opengl_swapbuffer();
}

uint64_t
game_update()
{
  return handle_message_loop_blocking(game_update_internal);
}