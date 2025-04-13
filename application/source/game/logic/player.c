/**
  *@file player.c
  *@author khalilhenoud@gmail.com
  *@brief first attempt at decompose logic into separate files.
  *@version 0.1
  *@date 2023-10-04
  *
  *@copyright Copyright (c) 2023
  *
 */
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <math/c/vector3f.h>
#include <math/c/matrix4f.h>
#include <entity/c/scene/camera.h>
#include <renderer/renderer_opengl.h>
#include <application/input.h>
#include <application/game/logic/player.h>
#include <application/game/logic/collision_utils.h>
#include <application/game/logic/bucket_processing.h>
#include <application/game/logic/camera.h>
#include <math/c/capsule.h>
#include <math/c/segment.h>
#include <math/c/face.h>
#include <math/c/sphere.h>
#include <entity/c/spatial/bvh.h>
#include <application/game/debug/face.h>
#include <application/game/debug/text.h>
#include <application/game/debug/flags.h>

#define KEY_MOVEMENT_MODE         '9'
#define KEY_SPEED_PLUS            '1'
#define KEY_SPEED_MINUS           '2'
#define KEY_JUMP                  0x20
#define KEY_STRAFE_LEFT           'A'
#define KEY_STRAFE_RIGHT          'D'
#define KEY_MOVE_FWD              'W'
#define KEY_MOVE_BACK             'S'
#define KEY_MOVE_UP               'Q'
#define KEY_MOVE_DOWN             'E'

#define REFERENCE_FRAME_TIME      0.033f
#define ITERATIONS                16 
#define LIMIT_DISTANCE            EPSILON_FLOAT_MIN_PRECISION


typedef
struct {
  vector3f velocity;
  vector3f velocity_limit;
  vector3f acceleration;
  float gravity;
  float friction;
  float jump_velocity;
  capsule_t capsule;
  uint32_t is_flying;
  uint32_t on_solid_floor;
  camera_t *camera;
  bvh_t *bvh;
} player_t;

static
player_t s_player;

////////////////////////////////////////////////////////////////////////////////
static
void
update_velocity(float delta_time)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  float friction = s_player.friction * multiplier;

  if (g_debug_flags.use_locked_motion)
    return;

  if (is_key_pressed(KEY_STRAFE_LEFT))
     s_player.velocity.data[0] -= s_player.acceleration.data[0] * multiplier;

  if (is_key_pressed(KEY_STRAFE_RIGHT))
     s_player.velocity.data[0] += s_player.acceleration.data[0] * multiplier;

  if (is_key_pressed(KEY_MOVE_FWD))
     s_player.velocity.data[2] += s_player.acceleration.data[2] * multiplier;

  if (is_key_pressed(KEY_MOVE_BACK))
     s_player.velocity.data[2] -= s_player.acceleration.data[2] * multiplier;

  if (s_player.is_flying) {
    if (is_key_pressed(KEY_MOVE_UP))
       s_player.velocity.data[1] += s_player.acceleration.data[1] * multiplier;

    if (is_key_pressed(KEY_MOVE_DOWN))
       s_player.velocity.data[1] -= s_player.acceleration.data[1] * multiplier;
  }

  {
    // apply friction.
    vector3f mul;
    vector3f_set_1f(&mul, 1.f);
    mul.data[0] = s_player.velocity.data[0] > 0.f ? -1.f : 1.f;
    mul.data[1] = s_player.velocity.data[1] > 0.f ? -1.f : 1.f;
    mul.data[2] = s_player.velocity.data[2] > 0.f ? -1.f : 1.f;

    s_player.velocity.data[0] += mul.data[0] * friction;
    s_player.velocity.data[0] = (mul.data[0] == -1.f) ? 
    fmax(s_player.velocity.data[0], 0.f) : fmin(s_player.velocity.data[0], 0.f);

    s_player.velocity.data[2] += mul.data[2] * friction;
    s_player.velocity.data[2] = (mul.data[2] == -1.f) ? 
    fmax(s_player.velocity.data[2], 0.f) : fmin(s_player.velocity.data[2], 0.f);

    if (s_player.is_flying) {
      s_player.velocity.data[1] += mul.data[1] * friction;
      s_player.velocity.data[1] = (mul.data[1] == -1.f) ? 
        fmax(s_player.velocity.data[1], 0.f) : 
        fmin(s_player.velocity.data[1], 0.f);
    }
  }

  {
    // apply limits.
    s_player.velocity.data[0] = 
      (s_player.velocity.data[0] < -s_player.velocity_limit.data[0]) ? 
      -s_player.velocity_limit.data[0] : s_player.velocity.data[0];
    s_player.velocity.data[0] = 
      (s_player.velocity.data[0] > +s_player.velocity_limit.data[0]) ? 
      +s_player.velocity_limit.data[0] : s_player.velocity.data[0];
  
    s_player.velocity.data[2] = 
      (s_player.velocity.data[2] < -s_player.velocity_limit.data[2]) ? 
      -s_player.velocity_limit.data[2] : s_player.velocity.data[2];
    s_player.velocity.data[2] = 
      (s_player.velocity.data[2] > +s_player.velocity_limit.data[2]) ? 
      +s_player.velocity_limit.data[2] : s_player.velocity.data[2];
  
    if (s_player.is_flying) {
      s_player.velocity.data[1] = 
        (s_player.velocity.data[1] < -s_player.velocity_limit.data[1]) ? 
        -s_player.velocity_limit.data[1] : s_player.velocity.data[1];
      s_player.velocity.data[1] = 
        (s_player.velocity.data[1] > +s_player.velocity_limit.data[1]) ?
        +s_player.velocity_limit.data[1] : s_player.velocity.data[1];
    }
  }
}

static
vector3f
get_world_relative_velocity(float delta_time)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  matrix4f cross_p;
  vector3f ortho_xz;
  vector3f relative = { 0.f, 0.f, 0.f };
  vector3f lookat_xz = { 0.f, 0.f, 0.f };
  float length;

  // cross the m_camera up vector with the flipped look at direction
  matrix4f_cross_product(&cross_p, &s_player.camera->up_vector);
  ortho_xz = mult_v3f(&s_player.camera->lookat_direction, -1.f);
  ortho_xz = mult_m4f_v3f(&cross_p, &ortho_xz);
  // only keep the xz components.
  ortho_xz.data[1] = 0;

  // we need ortho_xz.
  relative.data[0] += ortho_xz.data[0] * s_player.velocity.data[0] * multiplier;
  relative.data[2] += ortho_xz.data[2] * s_player.velocity.data[0] * multiplier;
  relative.data[1] += s_player.velocity.data[1] * multiplier;

  lookat_xz.data[0] = s_player.camera->lookat_direction.data[0];
  lookat_xz.data[2] = s_player.camera->lookat_direction.data[2];
  length = length_v3f(&lookat_xz);

  if (!IS_ZERO_MP(length)) {
    lookat_xz.data[0] /= length;
    lookat_xz.data[2] /= length;

    relative.data[0] += 
      lookat_xz.data[0] * s_player.velocity.data[2] * multiplier;
    relative.data[2] += 
      lookat_xz.data[2] * s_player.velocity.data[2] * multiplier;
  }

  return relative;
}

static
int32_t
get_floor_info(
  intersection_info_t collision_info[256], 
  uint32_t used)
{
  for (uint32_t i = 0; i < used; ++i)
    if (collision_info[i].flags == COLLIDED_FLOOR_FLAG)
      return (int32_t)i;
  
  return -1;
}

static
vector3f
get_adjusted_velocity(
  const vector3f velocity,
  const float toi, 
  const capsule_t *capsule,
  const float radius_scale)
{
  // if we are moving and there is intersection.
  if (!IS_ZERO_LP(length_squared_v3f(&velocity)) && toi != 1.f) {
    vector3f resultant = normalize_v3f(&velocity);
    mult_v3f(&resultant, capsule->radius  *radius_scale);
    add_set_v3f(&resultant, &velocity);
    return resultant;
  }

  return velocity;
}

static
int32_t
can_step_up(
  vector3f velocity,
  const float toi,
  float radius_scale,
  float *out_y)
{
  bvh_t *bvh = s_player.bvh;
  const capsule_t *capsule = &s_player.capsule;
  intersection_info_t collision_info[256];
  uint32_t info_used;
  int32_t floor_info_i;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };
  vector3f displacement = { 0.f, -(capsule->radius + 1) * 2, 0.f };
  capsule_t duplicate = *capsule;

  velocity = get_adjusted_velocity(velocity, toi, capsule, radius_scale);
  add_set_v3f(&duplicate.center, &velocity);

  // the capsule has to start in solid after applying the velocity.
  if (is_in_valid_space(bvh, &duplicate))
    return 0;

  {
    duplicate.center.data[1] -= displacement.data[1] / 2.f;
    // the capsule has to be in a non-solid state after the hike.
    if (!is_in_valid_space(bvh, &duplicate))
      return 0;

    info_used = get_time_of_impact(
      bvh, 
      &duplicate, 
      displacement, 
      collision_info, 
      16, 
      EPSILON_FLOAT_MIN_PRECISION);

    floor_info_i = get_floor_info(collision_info, info_used);
    if (floor_info_i != -1)
      info = collision_info[floor_info_i];

    if (info.flags == COLLIDED_FLOOR_FLAG) {
       float t;
       vector3f *normal = bvh->normals + info.bvh_face_index;
       face_t face = bvh->faces[info.bvh_face_index];
       face = get_extended_face(&face, capsule->radius  *2);

       {
         // the capsule must have cleared the above floor.
         vector3f penetration;
         point3f sphere_center;
         capsule_face_classification_t classify = 
           classify_capsule_face(
             &duplicate, &face, normal, 0, &penetration, &sphere_center);

         if (classify != CAPSULE_FACE_NO_COLLISION)
           return 0;
       }

       t = find_capsule_face_intersection_time(
        duplicate, 
        &face, 
        normal, 
        displacement, 
        16, 
        EPSILON_FLOAT_MIN_PRECISION);

       info.time = t;
       duplicate.center.data[1] += displacement.data[1]  *info.time;
       
       *out_y = duplicate.center.data[1];
       return 1;
    }
  }

  return 0;
}

static
collision_flags_t
handle_collision_detection(const vector3f displacement)
{
  bvh_t *bvh = s_player.bvh;
  capsule_t *capsule = &s_player.capsule;
  collision_flags_t flags = (collision_flags_t)0;
  intersection_info_t collision_info[256];
  uint32_t info_used;
  vector3f orientation = normalize_v3f(&displacement);
  vector3f velocity = displacement;
  float distance_to_go = length_v3f(&velocity);
  int32_t steps = 3;

  while (steps-- && !IS_ZERO_LP(length_squared_v3f(&velocity))) {
    info_used = get_time_of_impact(
      bvh, 
      capsule, 
      velocity,
      collision_info,  
      ITERATIONS, 
      LIMIT_DISTANCE);

    info_used = process_collision_info(
      bvh, &velocity, collision_info, info_used);

    if (!info_used) {
      add_set_v3f(&capsule->center, &velocity);
      return flags;
    } else {
      float toi = collision_info[0].time;
      float step_y = 0.f;
      collision_flags_t l_flags = COLLIDED_NONE;
      vector3f to_apply;

      vector3f normal = get_averaged_normal(
        bvh, 
        s_player.on_solid_floor, 
        collision_info, 
        info_used, 
        &l_flags);
      flags |= l_flags;

      to_apply = mult_v3f(&velocity, toi);
      add_set_v3f(&capsule->center, &to_apply);
      distance_to_go -= length_v3f(&to_apply);
      distance_to_go = fmax(distance_to_go, 0.f);

      if (
        (l_flags & COLLIDED_WALLS_FLAG) && 
        can_step_up(velocity, toi, 0.5f, &step_y)) {
        
        if (g_debug_flags.draw_status) {
          float value = capsule->center.data[1] - step_y;
          char text[512];
          memset(text, 0, sizeof(text));
          sprintf(text, "STEPUP %f", value);
          add_debug_text_to_frame(text, red, 400.f, 320.f);
        }

        capsule->center.data[1] = step_y;
      } else {
        vector3f subtract;
        float dot;
        normalize_set_v3f(&velocity);
        dot = dot_product_v3f(&velocity, &normal);
        subtract = mult_v3f(&normal, dot);
        diff_set_v3f(&velocity, &subtract);

        dot = fmax(dot_product_v3f(&orientation, &velocity), 0.f);
        mult_set_v3f(&velocity, distance_to_go * dot);
      }
    }
  }

  return flags;
}

static
void
update_vertical_velocity(float delta_time)
{
  if (s_player.is_flying)
    return;

  bvh_t *bvh = s_player.bvh;
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  intersection_info_t collision_info[256];
  uint32_t info_used;
  int32_t floor_info_i;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };
  vector3f displacement = { 0.f, 0.f, 0.f };
  const float y_trace_start_offset = s_player.capsule.radius;
  const float y_trace_end_offset = -s_player.capsule.radius * 2;
  // this is necessary as the time of first impact precision is 1 / iterations.
  displacement.data[1] = y_trace_end_offset + y_trace_end_offset / ITERATIONS;
  s_player.on_solid_floor = 0;

  {
    capsule_t duplicate = s_player.capsule;
    duplicate.center.data[1] += y_trace_start_offset;

    info_used = get_time_of_impact(
      bvh, 
      &duplicate, 
      displacement, 
      collision_info, 
      ITERATIONS, 
      LIMIT_DISTANCE);

    floor_info_i = get_floor_info(collision_info, info_used);
    if (floor_info_i != -1)
      info = collision_info[floor_info_i];

    if (info.flags == COLLIDED_FLOOR_FLAG) {
      float t;
      vector3f *normal = bvh->normals + info.bvh_face_index;
      face_t face = bvh->faces[info.bvh_face_index];
      face = get_extended_face(&face, s_player.capsule.radius * 2);

      t = find_capsule_face_intersection_time(
        duplicate,
        &face,
        normal,
        displacement,
        ITERATIONS,
        LIMIT_DISTANCE);

      info.time = t;
      s_player.on_solid_floor = 1;
    }
  }

  // snap the capsule to the nearst floor.
  if (s_player.on_solid_floor && s_player.velocity.data[1] <= 0.f) {
    s_player.velocity.data[1] = 0.f;
    s_player.capsule.center.data[1] += y_trace_start_offset;
    s_player.capsule.center.data[1] += displacement.data[1] * info.time;

    if (g_debug_flags.draw_status) {
      float distance = 
        displacement.data[1] * info.time + y_trace_start_offset;
      char text[512];
      memset(text, 0, sizeof(text));
      sprintf(text, "SNAPPING %f", distance);
      add_debug_text_to_frame(text, green, 400.f, 300.f);

      {
        uint32_t i = info.bvh_face_index;
        debug_color_t color = 
          is_floor(bvh, i) ? green : (is_ceiling(bvh, i)? white : blue);
        add_debug_face_to_frame(
          bvh->faces + i, 
          bvh->normals + i, 
          color, 1);
      }
    }
  }

  if (is_key_triggered(KEY_JUMP) && s_player.on_solid_floor) {
    s_player.on_solid_floor = 0;
    s_player.velocity.data[1] = s_player.jump_velocity;
  }

  if (!s_player.on_solid_floor) {
    s_player.velocity.data[1] -= s_player.gravity * multiplier;
    s_player.velocity.data[1] = 
      fmax(s_player.velocity.data[1], -s_player.velocity_limit.data[1]);
  }
}

////////////////////////////////////////////////////////////////////////////////
void
player_init(
  point3f player_start,
  float player_angle,
  camera_t *camera,
  bvh_t *bvh)
{
  vector3f at = player_start; at.data[2] -= 1.f;
  vector3f up = {0.f, 1.f, 0.f};
  camera_init(camera, player_start, at, up);
  
  s_player.acceleration.data[0] = s_player.acceleration.data[1] = 
  s_player.acceleration.data[2] = 5.f;
  s_player.velocity.data[0] = s_player.velocity.data[1] = 
  s_player.velocity.data[2] = 0.f;
  s_player.velocity_limit.data[0] = s_player.velocity_limit.data[1] = 
  s_player.velocity_limit.data[2] = 10.f;
  s_player.gravity = 1.f;
  s_player.friction = 2.f;
  s_player.jump_velocity = 10.f;
  s_player.capsule.center = player_start;
  s_player.capsule.half_height = 12.f;
  s_player.capsule.radius = 16.f;
  s_player.is_flying = 0;
  s_player.on_solid_floor = 0;
  s_player.camera = camera;
  s_player.bvh = bvh;

  if (!is_in_valid_space(bvh, &s_player.capsule)) 
    ensure_in_valid_space(bvh, &s_player.capsule);
}

void
player_update(float delta_time)
{
  // TODO: cap the detla time when debugging.
  delta_time = fmin(delta_time, REFERENCE_FRAME_TIME);

  if (is_key_pressed(KEY_SPEED_PLUS)) {
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] += 0.25f;
  }

  if (is_key_pressed(KEY_SPEED_MINUS)) {
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] -= 0.25f;
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] = 
      fmax(s_player.velocity_limit.data[2], 0.125f);
  }

  if (is_key_triggered(KEY_MOVEMENT_MODE))
    s_player.is_flying = !s_player.is_flying;
  
  camera_update(s_player.camera, delta_time);
  update_velocity(delta_time);
  update_vertical_velocity(delta_time);

  {
    vector3f displacement = get_world_relative_velocity(delta_time);
    collision_flags_t flags = handle_collision_detection(displacement);

    // reset the vertical velocity if we collide with a ceiling
    if (
      !s_player.is_flying &&
      (flags & COLLIDED_CEILING_FLAG) == COLLIDED_CEILING_FLAG &&
      s_player.velocity.data[1] > 0.f)
      s_player.velocity.data[1] = 0.f;

    // set the camera position to follow the capsule
    s_player.camera->position = s_player.capsule.center;
  }

  {
    float y = 100.f;
    char array[256];
    snprintf(
      array, 256, 
      "CAPSULE POSITION:     %.2f     %.2f     %.2f", 
      s_player.capsule.center.data[0], 
      s_player.capsule.center.data[1], 
      s_player.capsule.center.data[2]);

    add_debug_text_to_frame(
      array, green, 0.f, (y+=20.f));
    add_debug_text_to_frame(
      "[9] SWITCH CAMERA MODE", 
      s_player.is_flying ? red : white, 0.f, (y+=20.f));
  }
}