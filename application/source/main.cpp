/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <windows.h>
#include <string>
#include <memory>
#include <cassert>
#include <application/application.h>
#include <application/platform/input_platform.h>
#include <library/misc/precision_timers.h>
#include <renderer/platform/opengl_platform.h>
#include <windowing/windowing.h>


std::unique_ptr<application> s_app = nullptr;

void update(void)
{
  s_app->update();
  opengl_swapbuffer();
}

int
main(int argc, char *argv[])
{
  // TODO: add config to control the client area size and the window style.
  int32_t screen_width = get_screen_width();
  int32_t screen_height = get_screen_height();
  int32_t client_width = (int32_t)(screen_width * 75.f / 100.f);
  int32_t client_height = (int32_t)(client_width / 16.f * 9.f);
  window_data_t data = create_window(
    "win_class_custom", 
    "engine", 
    client_width, 
    client_height);

  set_periodic_timers_resolution(1);

  input_parameters_t input_params{ (HWND*)&data.handle };
	input_set_client(&input_params);

  opengl_parameters_t params{ (HDC*)&data.device_context };
  opengl_initialize(&params);

  // remove emtpy spaces from the command lines.
  assert(argc >= 2);
  std::string cmd_args = argv[1];
  cmd_args.erase(
    std::remove_if(cmd_args.begin(), cmd_args.end(), std::isspace), 
    cmd_args.end());

	s_app = std::make_unique<application>(
    client_width, client_height, cmd_args.c_str());

  uint64_t result = handle_message_loop_blocking(update);

  s_app.reset();
  opengl_cleanup();
  destroy_window(&data);

  end_periodic_timers_resolution(1);

  return result;
}
