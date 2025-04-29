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
#include <math/c/face.h>
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
  float snap_velocity;
  float snap_shift;
  capsule_t capsule;
  uint32_t is_flying;
  uint32_t on_solid_floor;
  camera_t *camera;
  bvh_t *bvh;
} player_t;

static
player_t s_player;

////////////////////////////////////////////////////////////////////////////////
inline
void
clamp(float *f, float min, float max)
{
  *f = (*f < min) ? min : *f;
  *f = (*f > max) ? max : *f;
}

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

  if (is_key_pressed(KEY_MOVE_UP) && s_player.is_flying)
    s_player.velocity.data[1] += s_player.acceleration.data[1] * multiplier;

  if (is_key_pressed(KEY_MOVE_DOWN) && s_player.is_flying)
    s_player.velocity.data[1] -= s_player.acceleration.data[1] * multiplier;

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
    // clamp the velocity.
    vector3f limit = s_player.velocity_limit;
    clamp(s_player.velocity.data + 0, -limit.data[0], limit.data[0]);
    clamp(s_player.velocity.data + 2, -limit.data[2], limit.data[2]);
    if (s_player.is_flying)
      clamp(s_player.velocity.data + 1, -limit.data[1], limit.data[1]);
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
    float vely = s_player.velocity.data[2];
    lookat_xz.data[0] /= length;
    lookat_xz.data[2] /= length;

    relative.data[0] += lookat_xz.data[0] * vely * multiplier;
    relative.data[2] += lookat_xz.data[2] * vely * multiplier;
  }

  return relative;
}

static
int32_t
get_floor_index(const intersection_data_t *collisions)
{
  for (uint32_t i = 0; i < collisions->count; ++i)
    if (collisions->hits[i].flags == COLLIDED_FLOOR_FLAG)
      return (int32_t)i;
  
  return -1;
}

// does a vertical sweep to determine if we can snap the provided capsule. the 
// sweep is from [+radius, -(diameter + (diameter/iterations))]. The error term
// is necessary due to the collision term imprecision.
static
int32_t
can_snap_vertically(
  capsule_t capsule, 
  intersection_info_t *info, 
  float *out_y)
{
  bvh_t *bvh = s_player.bvh;
  intersection_data_t collisions;
  int32_t floor_index;
  const float diameter = capsule.radius * 2;
  vector3f displacement = { 0.f, -(diameter + diameter / ITERATIONS), 0.f };

  info->time = 1.f;
  info->flags = COLLIDED_NONE;
  info->bvh_face_index = (uint32_t)-1;
  
  {
    capsule.center.data[1] += capsule.radius;
    // early out if the sweep starts in collision.
    if (!is_in_valid_space(bvh, &capsule))
      return 0;
    
    collisions.count = get_time_of_impact(
      bvh, 
      &capsule, 
      displacement, 
      collisions.hits, 
      ITERATIONS, 
      LIMIT_DISTANCE);

    floor_index = get_floor_index(&collisions);
    if (floor_index != -1)
      *info = collisions.hits[floor_index];

    // after the sweep if we collided with any floor face.
    if (info->flags == COLLIDED_FLOOR_FLAG) {
      float t;
      vector3f *normal = bvh->normals + info->bvh_face_index;
      face_t face = bvh->faces[info->bvh_face_index];
      face = get_extended_face(&face, capsule.radius * 2);

      {
         // the capsule must have cleared the extended face. since we are 
         // dealing with a capsule face, there might be no collision even if we
         // haven't cleared the floor face.
         vector3f penetration;
         point3f sphere_center;
         capsule_face_classification_t classify = 
           classify_capsule_face(
             &capsule, &face, normal, 0, &penetration, &sphere_center);

         if (classify != CAPSULE_FACE_NO_COLLISION)
           return 0;
       }

      t = find_capsule_face_intersection_time(
        capsule,
        &face,
        normal,
        displacement,
        ITERATIONS,
        LIMIT_DISTANCE);

      info->time = t;
      *out_y = capsule.center.data[1] + displacement.data[1] * info->time;
      return 1;
    }
  }

  return 0;
}

static
void
update_vertical_velocity(float delta_time)
{
  bvh_t *bvh = s_player.bvh;
  intersection_data_t collisions;
  int32_t floor_index;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };
  float out_y;
  
  if (
    can_snap_vertically(s_player.capsule, &info, &out_y) && 
    s_player.velocity.data[1] <= s_player.snap_velocity) {
    float copy_y = s_player.capsule.center.data[1];
    s_player.on_solid_floor = 1;
    s_player.velocity.data[1] = 0.f;
    s_player.capsule.center.data[1] = out_y;

    if (g_debug_flags.draw_status) {
      uint32_t i = info.bvh_face_index;
      debug_color_t color = get_debug_color(bvh, i);
      float distance = copy_y - out_y;
      char text[512];
      memset(text, 0, sizeof(text));
      sprintf(text, "SNAPPING %f", distance);
      add_debug_text_to_frame(text, green, 400.f, 300.f);
      add_debug_face_to_frame(bvh->faces + i, bvh->normals + i, color, 1);
    }
  } else
    s_player.on_solid_floor = 0;
}

static
collision_flags_t
handle_collision_detection(const vector3f displacement)
{
  bvh_t *bvh = s_player.bvh;
  capsule_t *capsule = &s_player.capsule;
  collision_flags_t flags = (collision_flags_t)0;
  intersection_data_t collisions;
  vector3f orientation = normalize_v3f(&displacement);
  vector3f velocity = displacement;
  float energy_left = length_v3f(&velocity);
  uint32_t steps = 3;

  while (steps-- && !IS_ZERO_LP(length_squared_v3f(&velocity))) {
    collisions.count = get_time_of_impact(
      bvh, 
      capsule, 
      velocity,
      collisions.hits,  
      ITERATIONS, 
      LIMIT_DISTANCE);

    collisions.count = process_collision_info(
      bvh, &velocity, collisions.hits, collisions.count);

    // early out if there is no collision
    if (!collisions.count) {
      add_set_v3f(&capsule->center, &velocity);
      return flags;
    } 

    {
      float toi = collisions.hits[0].time;
      float out_y;
      collision_flags_t l_flags = COLLIDED_NONE;
      intersection_info_t info;
      capsule_t copy;

      vector3f to_apply = mult_v3f(&velocity, toi);
      add_set_v3f(&capsule->center, &to_apply);
      energy_left = fmax(energy_left - length_v3f(&to_apply), 0.f);

      // This should be moved, the only reason we have it here is because we 
      // require l_flags.
      vector3f normal = get_averaged_normal(
        bvh, 
        s_player.on_solid_floor, 
        collisions.hits, 
        collisions.count, 
        &l_flags);
      flags |= l_flags;

      // for this next step we move the capsule by an extra 1/4 radius along 
      // velocity and check if we can snap upwards
      copy = *capsule;
      {
        vector3f unit = normalize_v3f(&velocity);
        mult_set_v3f(&unit, s_player.snap_shift);
        add_set_v3f(&copy.center, &unit);
      }

      if (
        (l_flags & COLLIDED_WALLS_FLAG) && 
        can_snap_vertically(copy, &info, &out_y)) {
        float value = capsule->center.data[1] - out_y;
        capsule->center.data[1] = out_y;

        if (g_debug_flags.draw_status) {
          char text[512];
          memset(text, 0, sizeof(text));
          sprintf(text, "STEPUP %f", value);
          add_debug_text_to_frame(text, red, 400.f, 320.f);
        }

        if (g_debug_flags.draw_step_up) {
          uint32_t i = info.bvh_face_index;
          face_t *face = bvh->faces + i;
          vector3f *normal = bvh->normals + i;
          debug_color_t color = get_debug_color(bvh, i);
          int32_t width = is_floor(bvh, i) ? 3 : 2;
          add_debug_face_to_frame(face, normal, color, width);
        }
      } else {
        vector3f subtract;
        float dot;
        normalize_set_v3f(&velocity);
        dot = dot_product_v3f(&velocity, &normal);
        subtract = mult_v3f(&normal, dot);
        diff_set_v3f(&velocity, &subtract);

        // loss is proportional to the deviation from the initial direction
        energy_left *= fmax(dot_product_v3f(&orientation, &velocity), 0.f);
        mult_set_v3f(&velocity, energy_left);
      }
    }
  }

  return flags;
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
  s_player.snap_velocity = 0.f;
  s_player.snap_shift = s_player.capsule.radius / 2.f;
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
  vector3f displacement;
  collision_flags_t flags;

  // TODO: cap the detla time when debugging.
  delta_time = fmin(delta_time, REFERENCE_FRAME_TIME);
  
  camera_update(s_player.camera, delta_time);
  update_velocity(delta_time);
  if (!s_player.is_flying) 
    update_vertical_velocity(delta_time);

  displacement = get_world_relative_velocity(delta_time);
  flags = handle_collision_detection(displacement);

  // set the camera position to follow the capsule
  s_player.camera->position = s_player.capsule.center;

  // apply gravity
  if (!s_player.on_solid_floor) {
    float multiplier = delta_time / REFERENCE_FRAME_TIME;
    s_player.velocity.data[1] -= s_player.gravity * multiplier;
    s_player.velocity.data[1] = 
      fmax(s_player.velocity.data[1], -s_player.velocity_limit.data[1]);
  }

  // jump
  if (is_key_triggered(KEY_JUMP) && s_player.on_solid_floor) {
    s_player.on_solid_floor = 0;
    s_player.velocity.data[1] = s_player.jump_velocity;
  }

  // reset the vertical velocity if we collide with a ceiling
  if (
    !s_player.is_flying &&
    (flags & COLLIDED_CEILING_FLAG) == COLLIDED_CEILING_FLAG &&
    s_player.velocity.data[1] > 0.f)
    s_player.velocity.data[1] = 0.f;

  // increase velocity limit
  if (is_key_pressed(KEY_SPEED_PLUS)) {
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] += 0.25f;
  }

  // decrease velocity limit.
  if (is_key_pressed(KEY_SPEED_MINUS)) {
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] -= 0.25f;
    s_player.velocity_limit.data[0] = 
    s_player.velocity_limit.data[1] = 
    s_player.velocity_limit.data[2] = 
      fmax(s_player.velocity_limit.data[2], 0.125f);
  }

  // trigger flying mode.
  if (is_key_triggered(KEY_MOVEMENT_MODE))
    s_player.is_flying = !s_player.is_flying;

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