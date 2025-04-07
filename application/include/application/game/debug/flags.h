/**
 * @file flags.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-04-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef DEBUG_FLAGS_H
#define DEBUG_FLAGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef
struct debug_flags_t {
  uint32_t draw_ignored_faces : 1;
  uint32_t disable_depth_debug : 1;
  uint32_t use_locked_motion : 1;
  uint32_t draw_collision_query : 1;
  uint32_t draw_collided_face : 1;
  uint32_t draw_status : 1;
} debug_flags_t;

extern debug_flags_t g_debug_flags;

void
update_debug_flags(void);

#ifdef __cplusplus
}
#endif

#endif