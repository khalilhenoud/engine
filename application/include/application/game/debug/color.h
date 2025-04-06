/**
 * @file color.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef DEBUG_COLOR_H
#define DEBUG_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif


typedef
struct debug_color_t {
  float data[4];
} debug_color_t;

extern debug_color_t red;
extern debug_color_t green;
extern debug_color_t blue;
extern debug_color_t yellow;
extern debug_color_t white;
extern debug_color_t black;

#ifdef __cplusplus
}
#endif

#endif