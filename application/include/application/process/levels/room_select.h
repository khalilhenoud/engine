/**
 * @file room_select.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef APP_ROOM_SELECT_H
#define APP_ROOM_SELECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef struct allocator_t allocator_t;

void
load_room_select(
  const char* data_set, 
  const char* room, 
  float width,
  float height,
  const allocator_t* allocator);

void
update_room_select(
  float dt_seconds, 
  uint64_t framerate, 
  const allocator_t* allocator);

void
unload_room_select(const allocator_t* allocator);

const char*
should_unload_room_select();

#ifdef __cplusplus
}
#endif

#endif