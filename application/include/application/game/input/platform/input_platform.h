/**
 * @file input_platform.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef INPUT_PLATFORM_H
#define INPUT_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#include <application/game/input/platform/win32/input_parameters.h>
#else
// TODO: Implement static assert for C using negative indices array.
#endif


#define KEYBOARD_KEY_COUNT 256

typedef 
struct {
  uint8_t state[KEYBOARD_KEY_COUNT];
} keyboard_state_t;

typedef
enum {
  MOUSE_KEY_LEFT,
  MOUSE_KEY_MIDDLE,
  MOUSE_KEY_RIGHT,
  MOUSE_KEY_COUNT
} mouse_keys; 

typedef 
struct {
  int16_t state[MOUSE_KEY_COUNT];
} mouse_state_t;

typedef
struct {
  int64_t x;
  int64_t y;
} input_point_t;

typedef
struct {
  int64_t left;
  int64_t right;
  int64_t top;
  int64_t bottom;
} input_rect_t;

void
input_set_client(const input_parameters_t* params);

void
get_client_rect(input_rect_t* rect);

void
get_keyboard_state(keyboard_state_t* keyboard);

void
get_mouse_state(mouse_state_t* mouse);

int32_t
show_mouse_cursor(int32_t show);

void
get_cursor_position(input_point_t* point);

void
set_cursor_position(int32_t x, int32_t y);

void
screen_to_client(const input_point_t input, input_point_t* out);

void
client_to_screen(const input_point_t input, input_point_t* out);


#ifdef __cplusplus
}
#endif

#endif