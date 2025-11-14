/**
 * @file input.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef INPUT_H
#define INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define KEYBOARD_KEY_COUNT 256


typedef 
struct keyboard_state_t {
  uint8_t state[KEYBOARD_KEY_COUNT];
} keyboard_state_t;

typedef
enum mouse_keys {
  MOUSE_KEY_LEFT,
  MOUSE_KEY_MIDDLE,
  MOUSE_KEY_RIGHT,
  MOUSE_KEY_COUNT
} mouse_keys;

typedef 
struct mouse_state_t {
  int16_t state[MOUSE_KEY_COUNT];
} mouse_state_t;

////////////////////////////////////////////////////////////////////////////////
void
input_update(void);


////////////////////////////////////////////////////////////////////////////////
int32_t 
is_key_pressed(int32_t key);
int32_t
is_key_triggered(int32_t key);


////////////////////////////////////////////////////////////////////////////////
int32_t 
is_mouse_left_pressed();
int32_t
is_mouse_left_triggered();

int32_t 
is_mouse_mid_pressed();
int32_t
is_mouse_mid_triggered();

int32_t 
is_mouse_right_pressed();
int32_t
is_mouse_right_triggered();

void
show_mouse_cursor(int32_t show);
void 
get_position(int32_t* x, int32_t* y);
void 
get_window_position(int32_t* x, int32_t* y);
void 
set_window_position(int32_t x, int32_t y);
void 
center_cursor();

void
get_mouse_state(mouse_state_t* mouse);
void
input_set_client(const uintptr_t handle);


#ifdef __cplusplus
}
#endif

#endif
