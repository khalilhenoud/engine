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
};

#endif