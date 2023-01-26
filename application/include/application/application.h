/**
 * @file application.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef APPLICATION_H
#define APPLICATION_H

#include <cstdint>


struct 
application {

  application(
    int32_t width, 
    int32_t height, 
    const char* dataset);
  ~application();
    
  uint64_t 
  update();

  void 
  update_camera();

  //////////////////////////////////////////////////////////////////////////////
  std::string m_dataset;

  int32_t x   = 0,  y   = 0;
  int32_t dx  = 0,  dy  = 0;
  int32_t mouse_x       = 0,   mouse_y      = 0;
  int32_t prev_mouse_x  = -1,  prev_mouse_y = -1;
  float   y_limit       = 0.f;

  bool    m_disable_input = false;
};

#endif