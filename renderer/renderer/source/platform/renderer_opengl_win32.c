/**
 * @file renderer_opengl_win32.c
 * @author khalilhenoud@gmail.com
 * @brief the win32 specific part of the opengl renderer.
 * @version 0.1
 * @date 2023-01-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdint.h>
#include <renderer/platform/opengl_platform.h>


static 
const HDC* device_context;      // GDI specific, HANDLE, PVOID

static 
HGLRC rendering_context;        // WIN32 OpenGL specific (not a handle)

void 
opengl_initialize(const opengl_parameters_t* params)
{
  int32_t iPixelFormat = 0;
  PIXELFORMATDESCRIPTOR kPFD = { 0 };
  
  device_context = params->device_context;

  // Binding OpenGL to the current window.
  kPFD.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  kPFD.nVersion = 1;
  kPFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
  kPFD.dwLayerMask = PFD_MAIN_PLANE;
  kPFD.iPixelType = PFD_TYPE_RGBA;
  kPFD.cColorBits = 32;
  kPFD.cDepthBits = 32;

  iPixelFormat = ChoosePixelFormat(*device_context, &kPFD);
  SetPixelFormat(*device_context, iPixelFormat, &kPFD);

  rendering_context = wglCreateContext(*device_context);
  wglMakeCurrent(*device_context, rendering_context);
}

void 
opengl_swapbuffer()
{
  SwapBuffers(*device_context);
}

void 
opengl_cleanup()
{
  wglDeleteContext(rendering_context);
}