/**
 * @file opengl_platform.h
 * @author khalilhenoud@gmail.com
 * @brief separates the platform specific part of opengl from the agnostic part.
 * @version 0.1
 * @date 2023-01-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef OPENGL_PLATFORM_H
#define OPENGL_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <renderer/platform/win32/opengl_parameters.h>
#else
// TODO: Implement static assert for C using negative indices array.
#endif
#include <renderer/internal/module.h>


RENDERER_API
void 
opengl_initialize(const opengl_parameters_t* params);

RENDERER_API
void 
opengl_swapbuffer();

RENDERER_API
void 
opengl_cleanup();


#ifdef __cplusplus
}
#endif

#endif