/**
 * @file camera.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2025-04-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <assert.h>
#include <application/input.h>
#include <application/game/logic/camera.h>
#include <entity/c/scene/camera.h>
#include <math/c/matrix4f.h>

#define KEY_RESET_CAMERA          'C'


typedef
struct {
  float delta_x;
  float delta_y;
  float previous_x;
  float previous_y;
  float current_x;
  float current_y;
} cursor_info_t;

static
cursor_info_t
update_cursor(float delta_time)
{
  static int32_t previous_x = -1;
  static int32_t previous_y = -1;
  int32_t current_x, current_y;
  cursor_info_t info;

  // center the mouse cursor at first call.
  if (previous_x == -1) {
    center_cursor();
    get_position(&current_x, &current_y);
    previous_x = current_x;
    previous_y = current_y;
  } else
    get_position(&current_x, &current_y);

  // reset the cursor position.
  set_position(previous_x, previous_y);

  info.delta_x = current_x - previous_x;
  info.delta_y = current_y - previous_y;
  info.current_x = current_x;
  info.current_y = current_y;
  info.previous_x = previous_x;
  info.previous_y = previous_y;
  return info;
}

////////////////////////////////////////////////////////////////////////////////
void
camera_init(
  camera_t *camera,
  point3f position,
  vector3f look_at,
  vector3f up)
{
  assert(camera);
  camera_set_lookat(camera, position, look_at, up);
}

static
void
handle_input(camera_t *camera)
{
  if (is_key_pressed(KEY_RESET_CAMERA)) {
    vector3f center = { 0.f, 0.f, 0.f };
    vector3f at = { 0.f, 0.f, -1.f };
    vector3f up = { 0.f, 1.f, 0.f };
    camera_set_lookat(camera, center, at, up);
  }
}

static
void
update_orientation(
  camera_t *camera, 
  float delta_time, 
  const float vertical_sensitivity,
  const float horizontal_sensitivity,
  const float vertical_angle_limit_radians)
{
  // used to keep track of the current maximum offset.
  static float current_y = 0.f;
  matrix4f cross_p, rotation_x, axis_xz, result_mtx;
  float frame_dy, tmp_y;
  vector3f ortho_xz;
  cursor_info_t cursor = update_cursor(delta_time);

  // cross the m_camera up vector with the flipped look at direction
  matrix4f_cross_product(&cross_p, &camera->up_vector);
  ortho_xz = mult_v3f(&camera->lookat_direction, -1.f);
  // ortho_xz is now orthogonal to the up and look_at vector.
  ortho_xz = mult_m4f_v3f(&cross_p, &ortho_xz);
  // only keep the xz components.
  ortho_xz.data[1] = 0;

  // orthogonize the camera lookat direction in the xz plane.
  if (!IS_ZERO_MP(length_v3f(&ortho_xz))) {
    matrix4f_cross_product(&cross_p, &ortho_xz);
    camera->lookat_direction = mult_m4f_v3f(&cross_p, &camera->up_vector);
    camera->lookat_direction = mult_v3f(&camera->lookat_direction, -1.f);
  }

  // limit the rotation along y, we test against the limit.
  frame_dy = -cursor.delta_y * vertical_sensitivity;
  tmp_y = current_y + frame_dy;
  
  // limit the frame dy.
  frame_dy = (tmp_y > vertical_angle_limit_radians) ? 
    (vertical_angle_limit_radians - current_y) : frame_dy;
  frame_dy = (tmp_y < -vertical_angle_limit_radians) ? 
    (-vertical_angle_limit_radians - current_y) : frame_dy;
  current_y += frame_dy;

  // rotate the camera left and right
  matrix4f_rotation_y(&rotation_x, -cursor.delta_x * horizontal_sensitivity);
  // rotate the camera up and down
  matrix4f_set_axisangle(&axis_xz, &ortho_xz, TO_DEGREES(frame_dy));

  // switching the rotations here makes no difference, why? because visually the
  // result is the same. Simulate it using your thumb and index.
  result_mtx = mult_m4f(&rotation_x, &axis_xz);
  camera->lookat_direction = mult_m4f_v3f(
    &result_mtx, &camera->lookat_direction);
  camera->up_vector = mult_m4f_v3f(&result_mtx, &camera->up_vector);
}

void
camera_update(
  camera_t *camera, 
  float delta_time, 
  const float vertical_sensitivity,
  const float horizontal_sensitivity,
  const float vertical_angle_limit_radians)
{
  handle_input(camera);
  update_orientation(
    camera, 
    delta_time, 
    vertical_sensitivity, 
    horizontal_sensitivity, 
    vertical_angle_limit_radians);
}