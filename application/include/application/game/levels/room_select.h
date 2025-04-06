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


typedef struct level_t level_t;

void
construct_level_selector(level_t* level);

#ifdef __cplusplus
}
#endif

#endif