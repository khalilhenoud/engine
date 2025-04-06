/**
 * @file face.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-03-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <assert.h>
#include <string.h>
#include <application/process/debug/face.h>
#include <math/c/face.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>

#define DEBUG_FACES_MAX_COUNT     2048


typedef
struct {
  face_t face;
  vector3f normal;
  debug_color_t color;
  int32_t thickness;
} debug_face_t;

typedef
struct {
  debug_face_t faces[DEBUG_FACES_MAX_COUNT];
  uint32_t used;
} debug_face_frame_t;

////////////////////////////////////////////////////////////////////////////////
static debug_face_frame_t debug_frame;

void
add_debug_face_to_frame(
  face_t *face, 
  vector3f *normal,
  debug_color_t color, 
  int32_t thickness)
{
  assert(face && normal && "face or normal is null!");
  assert(
    debug_frame.used < DEBUG_FACES_MAX_COUNT && 
    "reached the limit of debug faces!");

  {
    debug_face_t *debug_face = debug_frame.faces + debug_frame.used++;
    debug_face->face = *face;
    debug_face->normal = *normal;
    debug_face->color = color;
    debug_face->thickness = thickness;
  }
}

static
void
draw_debug_face(
  face_t *face, 
  vector3f *normal, 
  debug_color_t *debug_color, 
  int32_t thickness,
  pipeline_t *pipeline,
  const int32_t disable_depth)
{
  const float mult = 0.1f;
  float vertices[12];
  color_t color;
  memcpy(color.data, debug_color->data, sizeof(color.data));

  vertices[0 * 3 + 0] = face->points[0].data[0] + normal->data[0] * mult;
  vertices[0 * 3 + 1] = face->points[0].data[1] + normal->data[1] * mult;
  vertices[0 * 3 + 2] = face->points[0].data[2] + normal->data[2] * mult;
  vertices[1 * 3 + 0] = face->points[1].data[0] + normal->data[0] * mult;
  vertices[1 * 3 + 1] = face->points[1].data[1] + normal->data[1] * mult;
  vertices[1 * 3 + 2] = face->points[1].data[2] + normal->data[2] * mult;
  vertices[2 * 3 + 0] = face->points[2].data[0] + normal->data[0] * mult;
  vertices[2 * 3 + 1] = face->points[2].data[1] + normal->data[1] * mult;
  vertices[2 * 3 + 2] = face->points[2].data[2] + normal->data[2] * mult;
  vertices[3 * 3 + 0] = face->points[0].data[0] + normal->data[0] * mult;
  vertices[3 * 3 + 1] = face->points[0].data[1] + normal->data[1] * mult;
  vertices[3 * 3 + 2] = face->points[0].data[2] + normal->data[2] * mult;

  if (disable_depth) {
    disable_depth_test();
    draw_lines(vertices, 4, color, thickness, pipeline);
    enable_depth_test();
  } else
    draw_lines(vertices, 4, color, thickness, pipeline);
}

void
draw_debug_face_frame(
  pipeline_t *pipeline,
  const int32_t disable_depth)
{
  for (uint32_t i = 0; i < debug_frame.used; ++i) {
    draw_debug_face(
      &debug_frame.faces[i].face, 
      &debug_frame.faces[i].normal, 
      &debug_frame.faces[i].color, 
      debug_frame.faces[i].thickness,
      pipeline,
      disable_depth);
  }

  debug_frame.used = 0;
}