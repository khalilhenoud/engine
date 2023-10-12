/**
 * @file camera.c
 * @author khalilhenoud@gmail.com
 * @brief first attempt at decompose logic into separate files.
 * @version 0.1
 * @date 2023-10-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdint.h>
#include <assert.h>
#include <math/c/vector3f.h>
#include <math/c/matrix4f.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <application/input.h>
#include <application/process/logic/camera.h>

#define KEY_SPEED_PLUS      '1'
#define KEY_SPEED_MINUS     '2'
#define KEY_STRAFE_LEFT     'A'
#define KEY_STRAFE_RIGHT    'D'
#define KEY_MOVE_FWD        'W'
#define KEY_MOVE_BACK       'S'
#define KEY_MOVE_UP         'Q'
#define KEY_MOVE_DOWN       'E'
#define KEY_RESET_CAMERA    'C'


static int32_t prev_mouse_x = -1; 
static int32_t prev_mouse_y = -1;

void
recenter_camera_cursor(void)
{
  prev_mouse_x = -1;
  prev_mouse_y = -1;
}

static
void
handle_input(camera_t* camera)
{
  static float    y_limit = 0.f;
  static float    speed = 10.f;
  
  matrix4f crossup, camerarotateY, camerarotatetemp, tmpmatrix;
  float dx, dy, d, tempangle;
  int32_t mouse_x, mouse_y = 0;
  
  // static bool falling = false;
  float length;
  int32_t mx, my;
  vector3f tmp, tmp2;

  if (is_key_pressed(KEY_RESET_CAMERA)) {
    y_limit = 0;
    vector3f center;
    center.data[0] = center.data[1] = center.data[2] = 0.f;
    vector3f at;
    at.data[0] = at.data[1] = at.data[2] = 0.f;
    at.data[2] = -1.f;
    vector3f up;
    up.data[0] = up.data[1] = up.data[2] = 0.f;
    up.data[1] = 1.f;
    set_camera_lookat(camera, center, at, up);
  }

  if (is_key_pressed(KEY_SPEED_PLUS)) {
    speed += 0.25;
  }

  if (is_key_pressed(KEY_SPEED_MINUS)) {
    speed -= 0.25;
    speed = fmax(speed, 0.125);
  }

  if (prev_mouse_x == -1)
    center_cursor();

  // Handling the m_camera controls, first orientation, second translation.
  get_position(&mx, &my);
  mouse_x = mx;
  mouse_y = my;
  if (prev_mouse_x == -1)
    prev_mouse_x = mx;
  if (prev_mouse_y == -1)
    prev_mouse_y = my;
  dx = (float)mouse_x - prev_mouse_x;
  dy = (float)mouse_y - prev_mouse_y;
  set_position(prev_mouse_x, prev_mouse_y);

  // Crossing the m_camera up vector with the opposite of the look at direction.
  matrix4f_cross_product(&crossup, &camera->up_vector);
  tmp = mult_v3f(&camera->lookat_direction, -1.f);
  // 'tmp' is now orthogonal to the up and look_at vector.
  tmp = mult_m4f_v3f(&crossup, &tmp);

  // Orthogonalizing the m_camera up and direction vector (to avoid floating
  // point imprecision). Discard the y and the w components (preserver x and z,
  // making it parallel to the movement plane).
  tmp.data[1] = 0;
  length = sqrtf(tmp.data[0] * tmp.data[0] + tmp.data[2] * tmp.data[2]);
  if (length != 0) {
    // Normalizes tmp and return cross product matrix.
    matrix4f_cross_product(&crossup, &tmp);
    tmp2 = camera->up_vector;
    // 'tmp2' is now the oppposite of the direction vector.
    tmp2 = mult_m4f_v3f(&crossup, &tmp2);
    camera->lookat_direction = mult_v3f(&tmp2, -1.f);
  }

  // Create a rotation matrix on the y relative the movement of the mouse on x.
  matrix4f_rotation_y(&camerarotateY, -dx / 1000.f);
  // Limit y_limit to a certain range (to control x range of movement).
  tempangle = y_limit;
  tempangle -= dy / 1000;
  d = -dy / 1000;
  if (tempangle > 0.925f)
    d = 0.925f - y_limit;
  else if (tempangle < -0.925f)
    d = -0.925f - y_limit;
  y_limit += d;
  // Find the rotation matrix along perpendicular axis.
  matrix4f_set_axisangle(&camerarotatetemp, &tmp, d / K_PI * 180);
  // Switching the rotations here makes no difference, why? It seems
  // geometrically the result is the same. Just simulate it using your thumb and
  // index.
  tmpmatrix = mult_m4f(&camerarotateY, &camerarotatetemp);
  camera->lookat_direction = mult_m4f_v3f(
    &tmpmatrix, &camera->lookat_direction);
  camera->up_vector = mult_m4f_v3f(&tmpmatrix, &camera->up_vector);

  // Handling translations.
  if (is_key_pressed(KEY_MOVE_DOWN)) {
    camera->position.data[1] -= speed;
  }

  if (is_key_pressed(KEY_MOVE_UP)) {
    camera->position.data[1] += speed;
  }

  if (is_key_pressed(KEY_STRAFE_LEFT)) {
    camera->position.data[0] -= tmp.data[0] * speed;
    camera->position.data[2] -= tmp.data[2] * speed;
  }

  if (is_key_pressed(KEY_STRAFE_RIGHT)) {
    camera->position.data[0] += tmp.data[0] * speed;
    camera->position.data[2] += tmp.data[2] * speed;
  }

  if (is_key_pressed(KEY_MOVE_FWD)) {
    vector3f vecxz;
    vecxz.data[0] = camera->lookat_direction.data[0];
    vecxz.data[2] = camera->lookat_direction.data[2];
    length = 
      sqrtf(vecxz.data[0] * vecxz.data[0] + vecxz.data[2] * vecxz.data[2]);
    if (length != 0) {
      vecxz.data[0] /= length;
      vecxz.data[2] /= length;
      camera->position.data[0] += vecxz.data[0] * speed;
      camera->position.data[2] += vecxz.data[2] * speed;
    }
  }

  if (is_key_pressed(KEY_MOVE_BACK)) {
    vector3f vecxz;
    vecxz.data[0] = camera->lookat_direction.data[0];
    vecxz.data[2] = camera->lookat_direction.data[2];
    length = 
      sqrtf(vecxz.data[0] * vecxz.data[0] + vecxz.data[2] * vecxz.data[2]);
    if (length != 0) {
      vecxz.data[0] /= length;
      vecxz.data[2] /= length;
      camera->position.data[0] -= vecxz.data[0] * speed;
      camera->position.data[2] -= vecxz.data[2] * speed;
    }
  }
}

void
camera_update(camera_t* camera)
{
  handle_input(camera);

#if COLLISION_LOGIC
  {
    auto&& draw_face = [&](uint32_t face_index) {
      float vertices[12];
      vertices[0 * 3 + 0] = collision_faces[face_index].points[0].data[0] + collision_normals[face_index].data[0] * 1;
      vertices[0 * 3 + 1] = collision_faces[face_index].points[0].data[1] + collision_normals[face_index].data[1] * 1;
      vertices[0 * 3 + 2] = collision_faces[face_index].points[0].data[2] + collision_normals[face_index].data[2] * 1;
      vertices[1 * 3 + 0] = collision_faces[face_index].points[1].data[0] + collision_normals[face_index].data[0] * 1;
      vertices[1 * 3 + 1] = collision_faces[face_index].points[1].data[1] + collision_normals[face_index].data[1] * 1;
      vertices[1 * 3 + 2] = collision_faces[face_index].points[1].data[2] + collision_normals[face_index].data[2] * 1;
      vertices[2 * 3 + 0] = collision_faces[face_index].points[2].data[0] + collision_normals[face_index].data[0] * 1;
      vertices[2 * 3 + 1] = collision_faces[face_index].points[2].data[1] + collision_normals[face_index].data[1] * 1;
      vertices[2 * 3 + 2] = collision_faces[face_index].points[2].data[2] + collision_normals[face_index].data[2] * 1;
      vertices[3 * 3 + 0] = collision_faces[face_index].points[0].data[0] + collision_normals[face_index].data[0] * 1;
      vertices[3 * 3 + 1] = collision_faces[face_index].points[0].data[1] + collision_normals[face_index].data[1] * 1;
      vertices[3 * 3 + 2] = collision_faces[face_index].points[0].data[2] + collision_normals[face_index].data[2] * 1;
      draw_lines(vertices, 4, { 1.f, 0.f, 0.f, 1.f }, 3, &pipeline);
    };

    auto&& is_falling = [&](capsule_t to_test, mathvector3f pos)-> bool {
      to_test.center = pos.data;
      for (uint32_t i = 0; i < collision_faces.size(); ++i) {
        if (!is_face_valid[i])
          continue;

        // won't consider any face who's normal isn't within a certain range.
        // this can be cached for our purposes
        if (!is_floor[i])
          continue;

        vector3f any;
        capsule_face_classification_t classification =
          classify_capsule_faceplane(
            &to_test,
            &collision_faces[i],
            &collision_normals[i],
            &any);

        if (classification != CAPSULE_FACE_NO_COLLISION)
          return false;
      }

      return true;
    };

#if COLLISION_LOGIC_PHYSICS
    // jump when space is pressed.
    if (is_key_triggered(0x20) /*&& !falling*/) {
      falling = true;
      y_speed = 20.f;
    }

    // TODO: Investigate why this keeps falling at at certain point.
    // TODO: Investigate the jitter.
    if (falling) 
    {
      y_speed -= y_acc;
      y_speed = max(y_speed, -20);
      m_camera.m_position.y += y_speed;
    }

    falling = is_falling(capsule, m_camera.m_position);
#endif

    capsule.center = m_camera.m_position.data;
    vector3f penetration;
    segment_t coplanar_overlap;
    capsule_face_classification_t classification;

    for (uint32_t i = 0; i < collision_faces.size(); ++i) {
      if (!is_face_valid[i])
        continue;

      capsule_face_classification_t classification =
        classify_capsule_faceplane(
          &capsule,
          &collision_faces[i],
          &collision_normals[i],
          &penetration);

      if (classification != CAPSULE_FACE_NO_COLLISION) {
        add_set_v3f(&capsule.center, &penetration);
        add_set_v3f(&m_camera.m_position.data, &penetration);
        draw_face(i);
      }
    }
  }
#endif
}