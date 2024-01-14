/**
 * @file level.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef LEVEL_H
#define LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef struct allocator_t allocator_t;

typedef
struct level_viewport_t {
  int32_t x, y, width, height;
} level_viewport_t;

typedef
struct level_context_t {
  const char* data_set;
  const char* level;
  level_viewport_t viewport;
} level_context_t;

typedef
void (*func_load)(const level_context_t context, const allocator_t* allocator);

typedef
void (*func_update)(const allocator_t* allocator);

typedef
void (*func_unload)(const allocator_t* allocator);

typedef
uint32_t (*func_should_unload)(void);

typedef
struct level_t {
  func_load load;
  func_update update;
  func_unload unload;
  func_should_unload should_unload;
} level_t;

#ifdef __cplusplus
}
#endif

#endif