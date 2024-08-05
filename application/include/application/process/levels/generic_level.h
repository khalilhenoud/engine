/**
 * @file generic_level.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef APP_GENERIC_LEVEL_H
#define APP_GENERIC_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct level_t level_t;

// TODO: implement a factory construction system, this is why we need it.
void
construct_generic_level(level_t* level);

#ifdef __cplusplus
}
#endif

#endif