/**
 * @file framerate_controller.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef FRAMERATE_CONTROLLER_H
#define FRAMERATE_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BUFFER_TIME_MS                1


typedef
enum framerate_duration_t {
  FPS_DURATION_SECONDS,
  FPS_DURATION_MS,
  FPS_DURATION_COUNT
} framerate_duration_t;

typedef
struct framerate_controller_internal_t {
  double current_fps;

  // cast to DWORD when working with timegettime.
  uint64_t start;
  uint64_t end;
  uint64_t target_frame_ms;

  // performance counter specific
  uint64_t ticks_per_second;
  uint32_t freq_counters_supported;
} framerate_controller_internal_t;

typedef
struct framerate_controller_t {
  uint64_t target_fps;
  uint32_t locked;
  framerate_controller_internal_t internal;
} framerate_controller_t;


void
initialize_controller(
  framerate_controller_t* controller, 
  uint64_t target_fps, 
  uint32_t locked);

float
controller_start(framerate_controller_t* controller);

double
controller_end(framerate_controller_t* controller);

double
get_current_fps(framerate_controller_t* controller);

void
lock_fps(
  framerate_controller_t* controller, 
  uint64_t target_fps);

void
unlock_fps(framerate_controller_t* controller);


#include "framerate_controller.impl"

#ifdef __cplusplus
}
#endif

#endif
