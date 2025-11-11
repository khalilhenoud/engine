/**
 * @file game.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef GAME_H
#define GAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


void 
game_init(
  int32_t width,
  int32_t height,
  const char *data_set);

void
game_cleanup();

uint64_t
game_update();

#ifdef __cplusplus
}
#endif

#endif