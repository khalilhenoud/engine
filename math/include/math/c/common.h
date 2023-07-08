/**
 * @file common.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef C_COMMON_H
#define C_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES
#include <stdint.h>


#ifndef M_PI
#define K_PI 3.14159265358979323846
#else
#define K_PI M_PI
#endif

#define K_EQUAL_TO(A, B, EPSI)  (fabs((A) - (B)) <= EPSI)

#define TO_RADIANS(degrees) ((degrees) / 180.f * K_PI)
#define TO_DEGREES(radians) ((radians) / K_PI * 180.f)


#ifdef __cplusplus
}
#endif

#endif