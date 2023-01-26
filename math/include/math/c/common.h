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


#ifdef __cplusplus
}
#endif

#endif