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
show_cursor(int32_t show);
void 
get_position(int32_t* x, int32_t* y);
void 
get_window_position(int32_t* x, int32_t* y);
void 
set_position(int32_t x, int32_t y);
void 
set_window_position(int32_t x, int32_t y);
void 
center_cursor();

#ifdef __cplusplus
}
#endif

#endif
