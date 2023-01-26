/**
 * @file pipeline.h
 * @author khalilhenoud@gmail.com
 * @brief provides transformation stack for the rendering pipeline.
 * @version 0.1
 * @date 2023-01-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef PIPELINE_H
#define PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math/c/matrix4f.h>


// TODO: implement a texture stack.
#define MODELVIEW_STACK   256
#define PROJECTION_STACK  10

typedef
enum {
  MODELVIEW,
  PROJECTION
} stack_mode_t;

typedef
enum {
  ORTHOGRAPHIC,
  PERSPECTIVE
} projection_mode_t;

typedef
enum {
  LEFT,
  RIGHT,
  TOP,
  BOTTOM,
  ZNEAR,
  ZFAR,
  FRUSTUM_COUNT
} frustum_t;

typedef
enum {
  X,
  Y,
  WIDTH,
  HEIGHT,
  VIEWPORT_COUNT
} viewport_t;

typedef 
struct {
  stack_mode_t mode;
  matrix4f modelview_stack[MODELVIEW_STACK];
  int32_t modelview_index;
  matrix4f projection_stack[PROJECTION_STACK];
  int32_t projection_index;
  projection_mode_t projection_mode;
  float frustum[FRUSTUM_COUNT];
  float viewport[VIEWPORT_COUNT];
  
  // internal use.
  matrix4f* current_stack;
  int32_t* current_index;
  int32_t current_max_index;
} pipeline_t;


////////////////////////////////////////////////////////////////////////////////
inline 
void 
set_matrix_mode(pipeline_t*, stack_mode_t);

inline
void
pipeline_set_default(pipeline_t* dst)
{
  dst->modelview_index = 0;
  dst->projection_index = 0;
  dst->projection_mode = PERSPECTIVE;
  memset(dst->frustum, 0, sizeof(float) * 6);
  memset(dst->viewport, 0, sizeof(float) * 4);

  set_matrix_mode(dst, MODELVIEW);
}

////////////////////////////////////////////////////////////////////////////////
inline
projection_mode_t
get_projection_type(const pipeline_t* pipeline)
{
  return pipeline->projection_mode;
}

/// @brief given the pipeline populate the parameters with the viewport
/// properties.
inline
void
get_viewport_info(
  const pipeline_t* pipeline, 
  float* x, 
  float*y, 
  float* width, 
  float* height)
{
  *x = pipeline->viewport[X];
  *y = pipeline->viewport[Y];
  *width = pipeline->viewport[WIDTH];
  *height = pipeline->viewport[HEIGHT];
}

inline
void
set_viewport(
  pipeline_t* dst, 
  float x, 
  float y, 
  float width, 
  float height)
{
  dst->viewport[X] = x;
  dst->viewport[Y] = y;
  dst->viewport[WIDTH] = width;
  dst->viewport[HEIGHT] = height;
}

/// @brief specify the frustum dimensions used for the perspective projection.
/// this also sets the projection type to perspective.
/// @param right The right frustum clipping plane on the near plane.
/// @param left The left frustum clipping plane on the near plane.
/// @param bottom	The bottom frustum clipping plane on the near plane.
/// @param top The top frustum clipping plane on the near plane.
/// @param near_z The near clipping plane distance (positive value).
/// @param far_z The far clipping plane distance (positive value).
inline
void
set_perspective(
  pipeline_t* dst, 
  float left, 
  float right, 
  float bottom, 
  float top, 
  float near_z, 
  float far_z)
{
  dst->projection_mode  = PERSPECTIVE;
  dst->frustum[LEFT]    = left;
  dst->frustum[RIGHT]   = right;
  dst->frustum[BOTTOM]  = bottom;
  dst->frustum[TOP]     = top;
  dst->frustum[ZNEAR]   = near_z;
  dst->frustum[ZFAR]    = far_z;
}

/// @brief the equivalent to set_perspective, but uses orthographic projection.
inline
void
set_orthographic(
  pipeline_t* dst, 
  float left, 
  float right, 
  float bottom, 
  float top, 
  float near_z, 
  float far_z)
{
  dst->projection_mode  = ORTHOGRAPHIC;
  dst->frustum[LEFT]    = left;
  dst->frustum[RIGHT]   = right;
  dst->frustum[BOTTOM]  = bottom;
  dst->frustum[TOP]     = top;
  dst->frustum[ZNEAR]   = near_z;
  dst->frustum[ZFAR]    = far_z;
}

/// @brief retrieves the information set by set_perspective or set_orthographic.
inline
void
get_frustum(
  const pipeline_t* pipeline, 
  float* left, 
  float* right, 
  float* bottom, 
  float* top, 
  float* near_z, 
  float* far_z)
{
  *left    = pipeline->frustum[LEFT];
  *right   = pipeline->frustum[RIGHT];
  *bottom  = pipeline->frustum[BOTTOM];
  *top     = pipeline->frustum[TOP];
  *near_z  = pipeline->frustum[ZNEAR];
  *far_z   = pipeline->frustum[ZFAR];
}

/// @brief sets the current stack mode of the pipeline, along with a few helper
/// variables. 
inline
void
set_matrix_mode(pipeline_t* dst, stack_mode_t mode)
{
  dst->mode = mode;
  if (mode == MODELVIEW) {
    dst->current_stack = dst->modelview_stack;
    dst->current_index = &dst->modelview_index;
    dst->current_max_index = MODELVIEW_STACK;
  } else if (mode == PROJECTION) {
    dst->current_stack = dst->projection_stack;
    dst->current_index = &dst->projection_index;
    dst->current_max_index = PROJECTION_STACK;
  }
}

/// @brief returns the current top of the stack matrix. 
inline
matrix4f
get_matrix(const pipeline_t* pipeline)
{
  return pipeline->current_stack[*pipeline->current_index];
}

/// @brief dupliate the matrix at the top of the stack and push it on top. this
/// effectively makes the matrix at the top of the stack identical to the one
/// just below it. this is useful for throwaway transformation cases. 
inline
void
push_matrix(pipeline_t* dst)
{
  assert((*dst->current_index + 1) < dst->current_max_index);
  dst->current_stack[*dst->current_index + 1] = dst->current_stack[*dst->current_index];
  ++*dst->current_index;
}

/// @brief removes and returns the top matrix. 
inline
matrix4f
pop_matrix(pipeline_t* dst)
{
  assert(dst->current_index > 0);
  return dst->current_stack[(*dst->current_index)--];
}

/// @brief loads the identity matrix at the top of the stack. 
inline
void
load_identity(pipeline_t* dst)
{
  matrix4f_set_identity(&dst->current_stack[*dst->current_index]);
}

/// @brief replaces the top matrix with src. 
inline
void
replace(pipeline_t* dst, const matrix4f* src)
{
  matrix4f_copy(&dst->current_stack[*dst->current_index], src);
}

/// @brief post multiply the current matrix at the top of the stack with the new
/// @a matrix, meaning the transformation that @a matrix represent will be
/// applied after the transformation represented by the matrix on top of the
/// stack.
/// @param matrix the transformation to post apply to the top of the stack.
inline
void
post_multiply(pipeline_t* dst, const matrix4f* matrix)
{
  matrix4f result = mult_m4f(matrix, dst->current_stack + *dst->current_index);
  matrix4f_copy(dst->current_stack + *dst->current_index, &result);
}

inline
void
post_rotate_x(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_x(&result, angle_radian);
  post_multiply(dst, &result);
}

inline
void
post_rotate_y(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_y(&result, angle_radian);
  post_multiply(dst, &result);
}

inline
void
post_rotate_z(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_z(&result, angle_radian);
  post_multiply(dst, &result);
}

inline
void
post_translate(pipeline_t* dst, float x, float y, float z)
{
  matrix4f result;
  matrix4f_translation(&result, x, y, z);
  post_multiply(dst, &result);
}

inline
void
post_scale(pipeline_t* dst, float x, float y, float z)
{
  matrix4f result;
  matrix4f_scale(&result, x, y, z);
  post_multiply(dst, &result);
}

/// @brief pre-multiply the top of the stack with @a matrix. @a matrix
/// transformation is applied before that represented by the top matrix.
/// @param matrix the pre-transformation to apply.
inline
void
pre_multiply(pipeline_t* dst, const matrix4f* matrix)
{
  mult_set_m4f(dst->current_stack + *dst->current_index, matrix);
}

inline
void
pre_rotate_x(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_x(&result, angle_radian);
  pre_multiply(dst, &result);
}

inline
void
pre_rotate_y(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_y(&result, angle_radian);
  pre_multiply(dst, &result);
}

inline
void
pre_rotate_z(pipeline_t* dst, float angle_radian)
{
  matrix4f result;
  matrix4f_rotation_z(&result, angle_radian);
  pre_multiply(dst, &result);
}

inline
void
pre_translate(pipeline_t* dst, float x, float y, float z)
{
  matrix4f result;
  matrix4f_translation(&result, x, y, z);
  pre_multiply(dst, &result);
}

inline
void
pre_scale(pipeline_t* dst, float x, float y, float z)
{
  matrix4f result;
  matrix4f_scale(&result, x, y, z);
  pre_multiply(dst, &result);
}


#ifdef __cplusplus
}
#endif

#endif
