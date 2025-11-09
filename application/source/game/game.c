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
#include <application/game/game.h>
#include <application/game/levels/generic_level.h>
#include <application/game/levels/room_select.h>
#include <application/game/memory_tracking/memory_tracking.h>
#include <entity/c/level/level.h>
#include <library/allocator/allocator.h>
#include <renderer/renderer_opengl.h>


char to_load[256];

void
set_level_to_load(const char* source)
{
  memset(to_load, 0, sizeof(to_load));
  memcpy(to_load, source, strlen(source));
}

level_t level;
allocator_t allocator;

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
  renderer_initialize();

  track_allocator_memory(&allocator);

  viewport_width = width;
  viewport_height = height;
  data_set = dataset;
  in_level_select = 1;

  construct_level();
  level.load(get_context(), &allocator);
}

void
game_cleanup()
{
  level.unload(&allocator);
  renderer_cleanup();
  ensure_no_leaks();
  in_level_select = !in_level_select;
}

void
game_update()
{
  level.update(&allocator);

  if (level.should_unload()) {
    game_cleanup();
    construct_level();
    level.load(get_context(), &allocator);
  }
}