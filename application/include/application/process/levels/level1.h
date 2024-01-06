/**
 * @file level1.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef APP_LEVEL1_H
#define APP_LEVEL1_H

#ifdef __cplusplus
extern "C" {
#endif


// TODO: levels needs a context that context global systems like memory
// allocator and job managers.
typedef struct allocator_t allocator_t;

void
load_level(
  const char* data_set, 
  const char* room, 
  float width,
  float height,
  const allocator_t* allocator);

void
update_level(
  float dt_seconds, 
  uint64_t framerate, 
  const allocator_t* allocator);

void
unload_level(const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif