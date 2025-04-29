/**
 * @file flags.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-04-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <application/input.h>
#include <application/game/debug/flags.h>
#include <application/game/debug/text.h>

#define KEY_COLLISION_QUERY       '3'
#define KEY_COLLISION_FACE        '4'
#define KEY_DRAW_STATUS           '5'
#define KEY_DRAW_IGNORED_FACES    '6'
#define KEY_DISABLE_DEPTH         '7'
#define KEY_MOVEMENT_LOCK         '8'
#define KEY_DRAW_STEP_UP_FACE     'P'


debug_flags_t g_debug_flags;

void
push_debug_flags_to_text_frame(void)
{
  float y = 150.f;

  add_debug_text_to_frame(
    "[3] RENDER COLLISION QUERIES", 
    g_debug_flags.draw_collision_query ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[4] RENDER COLLISION FACE", 
    g_debug_flags.draw_collided_face ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[5] SHOW SNAPPING/FALLING STATE", 
    g_debug_flags.draw_status ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[6] DRAW IGNORED FACES", 
    g_debug_flags.draw_ignored_faces ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[7] DISABLE DEPTH TEST DEBUG", 
    g_debug_flags.disable_depth_debug ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[8] LOCK DIRECTIONAL MOTION", 
    g_debug_flags.use_locked_motion ? red : white, 0.f, (y+=20.f));
  add_debug_text_to_frame(
    "[P] DRAW STEP UP FACE", 
    g_debug_flags.draw_step_up ? red : white, 0.f, (y+=20.f));
}

void
update_debug_flags(void)
{
  if (is_key_triggered(KEY_COLLISION_QUERY))
    g_debug_flags.draw_collision_query = !g_debug_flags.draw_collision_query;

  if (is_key_triggered(KEY_DRAW_IGNORED_FACES))
    g_debug_flags.draw_ignored_faces = !g_debug_flags.draw_ignored_faces;

  if (is_key_triggered(KEY_COLLISION_FACE))
    g_debug_flags.draw_collided_face = !g_debug_flags.draw_collided_face;

  if (is_key_triggered(KEY_DRAW_STATUS))
    g_debug_flags.draw_status = !g_debug_flags.draw_status;

  if (is_key_triggered(KEY_MOVEMENT_LOCK))
    g_debug_flags.use_locked_motion = !g_debug_flags.use_locked_motion;

  if (is_key_triggered(KEY_DISABLE_DEPTH))
    g_debug_flags.disable_depth_debug = !g_debug_flags.disable_depth_debug;

  if (is_key_triggered(KEY_DRAW_STEP_UP_FACE))
    g_debug_flags.draw_step_up = !g_debug_flags.draw_step_up;

  push_debug_flags_to_text_frame();
}