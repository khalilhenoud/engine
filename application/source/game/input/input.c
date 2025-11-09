/**
 * @file input.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <string.h>
#include <application/game/input/input.h>
#include <application/game/input/platform/input_platform.h>


typedef
struct {
  uint8_t is_pressed;
  uint8_t is_triggered;
  uint8_t was_pressed;
  uint8_t reserved;
} key_state_t;

static
key_state_t s_keyboard_keys[KEYBOARD_KEY_COUNT];
static
key_state_t s_mouse_keys[MOUSE_KEY_COUNT];


////////////////////////////////////////////////////////////////////////////////
void
input_update(void)
{
  int32_t i = 0;

  {
    // keyboard section.
    keyboard_state_t ks;
    memset(&ks, 0, sizeof(keyboard_state_t));
    get_keyboard_state(&ks);

    for (i = 0; i < KEYBOARD_KEY_COUNT; ++i) {
      s_keyboard_keys[i].is_pressed = ks.state[i] & (1 << 7) ? 1 : 0;

      if (s_keyboard_keys[i].is_pressed) {
        if (s_keyboard_keys[i].was_pressed)
          s_keyboard_keys[i].is_triggered = 0;
        else {
          s_keyboard_keys[i].was_pressed = 1;
          s_keyboard_keys[i].is_triggered = 1;
        }
      }
      else {
        s_keyboard_keys[i].is_triggered = 0;
        s_keyboard_keys[i].was_pressed = 0;
      }
    }
  }

  {
    // mouse section.
    mouse_state_t ms;
    get_mouse_state(&ms);

    for (i = 0; i < MOUSE_KEY_COUNT; ++i) {
      s_mouse_keys[i].is_pressed = ms.state[i] & (1 << 15) ? 1 : 0;

      if (s_mouse_keys[i].is_pressed) {
        if (s_mouse_keys[i].was_pressed)
          s_mouse_keys[i].is_triggered = 0;
        else {
          s_mouse_keys[i].was_pressed = 1;
          s_mouse_keys[i].is_triggered = 1;
        }
      }
      else {
        s_mouse_keys[i].is_triggered = 0;
        s_mouse_keys[i].was_pressed = 0;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
int32_t 
is_key_pressed(int32_t key)
{
  return s_keyboard_keys[key].is_pressed;
}

int32_t
is_key_triggered(int32_t key)
{
  return s_keyboard_keys[key].is_triggered;
}


////////////////////////////////////////////////////////////////////////////////
int32_t 
is_mouse_left_pressed()
{
	return s_mouse_keys[0].is_pressed;
}

int32_t
is_mouse_left_triggered()
{
	return s_mouse_keys[0].is_triggered;
}

int32_t
is_mouse_mid_pressed()
{
	return s_mouse_keys[1].is_pressed;
}

int32_t
is_mouse_mid_triggered()
{
	return s_mouse_keys[1].is_triggered;
}

int32_t
is_mouse_right_pressed()
{
	return s_mouse_keys[2].is_pressed;
}

int32_t
is_mouse_right_triggered()
{
	return s_mouse_keys[2].is_triggered;
}

void 
show_cursor(int32_t show)
{
  if (show)
    while (show_mouse_cursor(show) < 0);
  else
    while (show_mouse_cursor(show) >= 0);
}

void 
get_position(int32_t* x, int32_t* y)
{
	input_point_t point;
  get_cursor_position(&point);
	*x = point.x;
	*y = point.y;
}

void 
get_window_position(int32_t* x, int32_t* y)
{
	input_point_t screen, client;
	get_cursor_position(&screen);
	screen_to_client(screen, &client);
	*x = client.x;
	*y = client.y;
}

void 
set_position(int32_t x, int32_t y)
{
	set_cursor_position(x, y);
}

void 
set_window_position(int32_t x, int32_t y)
{
  input_point_t screen, client;
  client.x = x;
  client.y = y;
	client_to_screen(client, &screen);
	set_position((int32_t)screen.x, (int32_t)screen.y);
}

void 
center_cursor()
{
	input_rect_t rect;
	get_client_rect(&rect);
	set_position((int32_t)(rect.left + rect.right)/2, (int32_t)(rect.top + rect.bottom)/2);
}