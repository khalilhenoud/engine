/**
 * @file application.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <windows.h>
#include <math.h>
#include <renderer/pipeline.h>
#include <renderer/renderer_opengl.h>


pipeline_t pipeline;
mesh_render_data_t mesh_data;
float vertices[] = { -50.f, 0.f, -500.f, 50.f, 0.f, -500.f, 0.f, 50.f, -500.f};
float normals[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
float uvs[] = { 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f };
uint32_t indices[] = {0, 1, 2};

void
app_initialize(int32_t width, int32_t height)
{
  renderer_initialize();
  pipeline_set_default(&pipeline);
  set_viewport(&pipeline, 0.f, 0.f, float(width), float(height));
  update_viewport(&pipeline);

  // "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
  float znear = 0.1f, zfar = 4000.f, aspect = (float)width / height;
  float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
  float fw = fh * aspect;
  set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
  update_projection(&pipeline);
  set_matrix_mode(&pipeline, MODELVIEW);
  load_identity(&pipeline);

  color_t red;
  red.data[0] = red.data[3] = 1.f;
  red.data[1] = red.data[2] = 0.f;
  mesh_data.vertices = vertices;
  mesh_data.vertex_count = 3;
  mesh_data.normals = normals;
  mesh_data.uv_coords = uvs;
  mesh_data.indices = indices;
  mesh_data.indices_count = 3;
  mesh_data.ambient = red;
  mesh_data.diffuse = red;
  mesh_data.specular = red;
}

void
app_update()
{
  static float z_forward = 0.f;
  static float x_axis = 0.f, z_axis = 0.f;
  static float rotation_y = 0;
  clear_color_and_depth_buffers();

  if (GetAsyncKeyState('W'))
    z_forward = 0.35f;
  else if (GetAsyncKeyState('S'))
    z_forward = -0.35f;
  else
    z_forward = 0.f;

  if (GetAsyncKeyState('A'))
    rotation_y -= 0.002f;

  if (GetAsyncKeyState('D'))
    rotation_y += 0.002f;

  float z_add = sinf(rotation_y + K_PI/2);
  float x_add = cosf(rotation_y + K_PI/2);
  x_axis += x_add * z_forward;
  z_axis += z_add * z_forward;

  uint32_t t_data[1] = {0};
  t_data[0] = 0;
  push_matrix(&pipeline);
  post_translate(&pipeline, x_axis, -100.f, z_axis);
  post_rotate_y(&pipeline, rotation_y);
  draw_grid(&pipeline, 5000.f, 100);
  draw_meshes(&mesh_data, t_data, 1, &pipeline);
  pop_matrix(&pipeline);

  flush_operations();
}

void
app_cleanup()
{
  renderer_cleanup(); 
}
