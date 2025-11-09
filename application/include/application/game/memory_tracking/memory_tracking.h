/**
 * @file memory_tracking.h
 * @author khalilhenoud@gmail.com
 * @brief simple memory tracking solution
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef MEMORY_TRACKING_H
#define MEMORY_TRACKING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <library/allocator/allocator.h>


void 
track_allocator_memory(allocator_t *allocator);

void
ensure_no_leaks(void);

#ifdef __cplusplus
}
#endif

#endif