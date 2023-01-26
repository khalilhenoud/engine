/**
 * @file input_platform.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <application/platform/input_platform.h>


static
HWND* g_window_handle;

void
input_set_client(const input_parameters_t* params)
{
  g_window_handle = params->window_handle;
}

void
get_client_rect(input_rect_t* rect)
{
  RECT winrect;
  assert(g_window_handle);
  GetWindowRect(*g_window_handle, &winrect);
  rect->left   = winrect.left;
  rect->right  = winrect.right;
  rect->top    = winrect.top;
  rect->bottom = winrect.bottom;
}

void
get_keyboard_state(keyboard_state_t* keyboard)
{
  assert(g_window_handle);
  GetKeyboardState(keyboard->state);
}

void
get_mouse_state(mouse_state_t* mouse)
{
  assert(g_window_handle);
  mouse->state[MOUSE_KEY_LEFT] = GetAsyncKeyState(VK_LBUTTON);
  mouse->state[MOUSE_KEY_MIDDLE] = GetAsyncKeyState(VK_MBUTTON);
  mouse->state[MOUSE_KEY_RIGHT] = GetAsyncKeyState(VK_RBUTTON);
}

int32_t
show_mouse_cursor(int32_t show)
{
  assert(g_window_handle);
  return ShowCursor(show);
}

void
get_cursor_position(input_point_t* point)
{
  POINT pn;
  assert(g_window_handle);
  GetCursorPos(&pn);
  point->x = pn.x;
  point->y = pn.y;
}

void
set_cursor_position(int32_t x, int32_t y)
{
  assert(g_window_handle);
  SetCursorPos(x, y);
}

void
screen_to_client(const input_point_t input, input_point_t* out)
{
  POINT pn;
  pn.x = input.x;
  pn.y = input.y;
  assert(g_window_handle);
  ScreenToClient(*g_window_handle, &pn);
  out->x = pn.x;
  out->y = pn.y;
}

void
client_to_screen(const input_point_t input, input_point_t* out)
{
  POINT pn;
  pn.x = input.x;
  pn.y = input.y;
  assert(g_window_handle);
  ClientToScreen(*g_window_handle, &pn);
  out->x = pn.x;
  out->y = pn.y;
}