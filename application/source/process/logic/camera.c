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
#include <math.h>
#include <math/c/vector3f.h>
#include <math/c/matrix4f.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <application/input.h>
#include <application/process/spatial/bvh.h>
#include <application/process/logic/camera.h>
#include <math/c/capsule.h>
#include <math/c/segment.h>
#include <math/c/face.h>
#include <math/c/sphere.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <application/process/text/utils.h>

#define KEY_SPEED_PLUS            '1'
#define KEY_SPEED_MINUS           '2'
#define KEY_COLLISION_QUERY       '3'
#define KEY_COLLISION_FACE        '4'
#define KEY_DRAW_STATUS           '5'
#define KEY_MOVEMENT_MODE         '9'
#define KEY_JUMP                  0x20
#define KEY_STRAFE_LEFT           'A'
#define KEY_STRAFE_RIGHT          'D'
#define KEY_MOVE_FWD              'W'
#define KEY_MOVE_BACK             'S'
#define KEY_MOVE_UP               'Q'
#define KEY_MOVE_DOWN             'E'
#define KEY_RESET_CAMERA          'C'

#define SNAP_THRESHOLD            0.1f
#define SNAP_EXTENT               25
#define SNAP_UP_EXTENT            10
#define PROTRUSION_RATIO          0.5f
#define SORTED_ITERATIONS         16
#define ITERATIONS                6
#define BINS                      5
#define BINNED_MICRO_DISTANCE     25
#define BINNED_MACRO_LIMIT        40

#define REFERENCE_FRAME_TIME      0.033f


static int32_t draw_collision_query = 0;
static int32_t draw_collided_face = 0;
static int32_t draw_status = 0;
static vector3f cam_acc = { 5.f, 5.f, 5.f };
static vector3f cam_speed = { 0.f, 0.f, 0.f };
static vector3f cam_speed_limit = { 10.f, 10.f, 10.f };
static const float gravity_acc = 1.f;
static const float friction_dec = 2.f;
static const float friction_k = 1.f;
static const float jump_speed = 10.f;

// 0 is walking, 1 is flying (not yet used).
static uint32_t flying = 0;

static color_t red = { 1.f, 0.f, 0.f, 1.f };
static color_t green = { 0.f, 1.f, 0.f, 1.f };
static color_t blue = { 0.f, 0.f, 1.f, 1.f };
static color_t yellow = { 1.f, 1.f, 0.f, 1.f };
static color_t white = { 1.f, 1.f, 1.f, 1.f };

typedef
enum {
  COLLIDED_FLOOR_FLAG = 1 << 0,
  COLLIDED_CEILING_FLAG = 1 << 1,
  COLLIDED_WALLS = 1 << 2,
  COLLIDED_NONE = 1 << 3
} collision_flags_t;

typedef
struct {
  float time;
  collision_flags_t flags;
  uint32_t bvh_face_index;
} intersection_info_t;

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

// returns a copy of the camera with updated orientation
static
camera_t
get_camera_orientation(
  float delta_time, 
  const cursor_info_t *cursor, 
  const camera_t *camera,
  const float vertical_sensitivity,
  const float horizontal_sensitivity,
  const float vertical_angle_limit_radians)
{
  // used to keep track of the current maximum offset.
  static float current_y = 0.f;
  const float y_limit = vertical_angle_limit_radians;
  camera_t copy;
  matrix4f cross_p, rotation_x, axis_xz, result_mtx;
  float frame_dy, tmp_y;
  vector3f ortho_xz;

  {
    copy.lookat_direction = camera->lookat_direction;
    copy.position = camera->position;
    copy.up_vector = camera->up_vector;
  }

  // cross the m_camera up vector with the flipped look at direction
  matrix4f_cross_product(&cross_p, &copy.up_vector);
  ortho_xz = mult_v3f(&copy.lookat_direction, -1.f);
  // ortho_xz is now orthogonal to the up and look_at vector.
  ortho_xz = mult_m4f_v3f(&cross_p, &ortho_xz);
  // only keep the xz components.
  ortho_xz.data[1] = 0;

  // orthogonize the camera lookat direction in the xz plane.
  if (!IS_ZERO_MP(length_v3f(&ortho_xz))) {
    matrix4f_cross_product(&cross_p, &ortho_xz);
    copy.lookat_direction = mult_m4f_v3f(&cross_p, &copy.up_vector);
    copy.lookat_direction = mult_v3f(&copy.lookat_direction, -1.f);
  }

  // limit the rotation along y, we test against the limit.
  frame_dy = -cursor->delta_y * vertical_sensitivity;
  tmp_y = current_y + frame_dy;
  
  // limit the frame dy.
  frame_dy = (tmp_y > y_limit) ? (y_limit - current_y) : frame_dy;
  frame_dy = (tmp_y < -y_limit) ? (-y_limit - current_y) : frame_dy;
  current_y += frame_dy;

  // rotate the camera left and right
  matrix4f_rotation_y(&rotation_x, -cursor->delta_x * horizontal_sensitivity);
  // rotate the camera up and down
  matrix4f_set_axisangle(&axis_xz, &ortho_xz, TO_DEGREES(frame_dy));

  // switching the rotations here makes no difference, why? because visually the
  // result is the same. Simulate it using your thumb and index.
  result_mtx = mult_m4f(&rotation_x, &axis_xz);
  copy.lookat_direction = mult_m4f_v3f(&result_mtx, &camera->lookat_direction);
  copy.up_vector = mult_m4f_v3f(&result_mtx, &camera->up_vector);
  return copy;
}

static
vector3f
get_velocity(
  float delta_time,
  const vector3f current_velocity,
  float friction_constant,
  const vector3f velocity_limits,
  const vector3f acceleration,
  const int32_t flying)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  float friction = friction_constant * multiplier;
  vector3f velocity = current_velocity;

  if (is_key_pressed(KEY_STRAFE_LEFT))
    velocity.data[0] -= acceleration.data[0] * multiplier;

  if (is_key_pressed(KEY_STRAFE_RIGHT))
    velocity.data[0] += acceleration.data[0] * multiplier;

  if (is_key_pressed(KEY_MOVE_FWD))
    velocity.data[2] += acceleration.data[2] * multiplier;

  if (is_key_pressed(KEY_MOVE_BACK))
    velocity.data[2] -= acceleration.data[2] * multiplier;

  if (flying) {
    if (is_key_pressed(KEY_MOVE_UP))
      velocity.data[1] += acceleration.data[1] * multiplier;

    if (is_key_pressed(KEY_MOVE_DOWN))
      velocity.data[1] -= acceleration.data[1] * multiplier;
  }

  {
    // apply friction.
    vector3f mul;
    vector3f_set_1f(&mul, 1.f);
    mul.data[0] = velocity.data[0] > 0.f ? -1.f : 1.f;
    mul.data[1] = velocity.data[1] > 0.f ? -1.f : 1.f;
    mul.data[2] = velocity.data[2] > 0.f ? -1.f : 1.f;

    velocity.data[0] += mul.data[0] * friction;
    velocity.data[0] = (mul.data[0] == -1.f) ? 
      fmax(velocity.data[0], 0.f) : fmin(velocity.data[0], 0.f);

    velocity.data[2] += mul.data[2] * friction;
    velocity.data[2] = (mul.data[2] == -1.f) ? 
      fmax(velocity.data[2], 0.f) : fmin(velocity.data[2], 0.f);

    if (flying) {
      velocity.data[1] += mul.data[1] * friction;
      velocity.data[1] = (mul.data[1] == -1.f) ? 
        fmax(velocity.data[1], 0.f) : fmin(velocity.data[1], 0.f);
    }
  }

  {
    // apply limits.
    velocity.data[0] = (velocity.data[0] < -velocity_limits.data[0]) ? 
      -velocity_limits.data[0] : velocity.data[0];
    velocity.data[0] = (velocity.data[0] > +velocity_limits.data[0]) ? 
      +velocity_limits.data[0] : velocity.data[0];
    
    velocity.data[2] = (velocity.data[2] < -velocity_limits.data[2]) ? 
      -velocity_limits.data[2] : velocity.data[2];
    velocity.data[2] = (velocity.data[2] > +velocity_limits.data[2]) ? 
      +velocity_limits.data[2] : velocity.data[2];
    
    if (flying) {
      velocity.data[1] = (velocity.data[1] < -velocity_limits.data[1]) ?
        -velocity_limits.data[1] : velocity.data[1];
      velocity.data[1] = (velocity.data[1] > +velocity_limits.data[1]) ?
        +velocity_limits.data[1] : velocity.data[1];
    }
  }

  return velocity;
}

static
vector3f
get_relative_displacement(
  float delta_time, 
  const camera_t* camera, 
  const vector3f velocity)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  matrix4f cross_p;
  vector3f ortho_xz;
  vector3f relative = { 0.f, 0.f, 0.f };
  vector3f lookat_xz = { 0.f, 0.f, 0.f };
  float length;

  // cross the m_camera up vector with the flipped look at direction
  matrix4f_cross_product(&cross_p, &camera->up_vector);
  ortho_xz = mult_v3f(&camera->lookat_direction, -1.f);
  ortho_xz = mult_m4f_v3f(&cross_p, &ortho_xz);
  // only keep the xz components.
  ortho_xz.data[1] = 0;

  // we need ortho_xz.
  relative.data[0] += ortho_xz.data[0] * velocity.data[0] * multiplier;
  relative.data[2] += ortho_xz.data[2] * velocity.data[0] * multiplier;
  relative.data[1] += velocity.data[1] * multiplier;

  lookat_xz.data[0] = camera->lookat_direction.data[0];
  lookat_xz.data[2] = camera->lookat_direction.data[2];
  length = length_v3f(&lookat_xz);

  if (!IS_ZERO_MP(length)) {
    lookat_xz.data[0] /= length;
    lookat_xz.data[2] /= length;

    relative.data[0] += lookat_xz.data[0] * velocity.data[2] * multiplier;
    relative.data[2] += lookat_xz.data[2] * velocity.data[2] * multiplier;
  }

  return relative;
}

static
void
handle_macro_keys(camera_t* camera)
{
  if (is_key_pressed(KEY_RESET_CAMERA)) {
    vector3f center = { 0.f, 0.f, 0.f };
    vector3f at = { 0.f, 0.f, 0.f };
    vector3f up = { 0.f, 0.f, 0.f };
    at.data[2] = -1.f;
    up.data[1] = 1.f;
    set_camera_lookat(camera, center, at, up);
  }

  if (is_key_triggered(KEY_COLLISION_QUERY))
    draw_collision_query = !draw_collision_query;

  if (is_key_triggered(KEY_COLLISION_FACE))
    draw_collided_face = !draw_collided_face;

  if (is_key_triggered(KEY_DRAW_STATUS))
    draw_status = !draw_status;

  if (is_key_pressed(KEY_SPEED_PLUS))
    cam_speed_limit.data[0] = 
    cam_speed_limit.data[1] = 
    cam_speed_limit.data[2] += 0.25f;

  if (is_key_pressed(KEY_SPEED_MINUS)) {
    cam_speed_limit.data[0] = 
    cam_speed_limit.data[1] = 
    cam_speed_limit.data[2] -= 0.25f;
    cam_speed_limit.data[0] = 
    cam_speed_limit.data[1] = 
    cam_speed_limit.data[2] = fmax(cam_speed_limit.data[2], 0.125f);
  }

  if (is_key_triggered(KEY_MOVEMENT_MODE))
    flying = !flying;
}

static
void
draw_face(
  bvh_face_t* face, 
  vector3f* normal, 
  color_t* color, 
  int32_t thickness,
  pipeline_t* pipeline)
{
  float vertices[12];

  vertices[0 * 3 + 0] = face->points[0].data[0] + face->normal.data[0];
  vertices[0 * 3 + 1] = face->points[0].data[1] + face->normal.data[1];
  vertices[0 * 3 + 2] = face->points[0].data[2] + face->normal.data[2];
  vertices[1 * 3 + 0] = face->points[1].data[0] + face->normal.data[0];
  vertices[1 * 3 + 1] = face->points[1].data[1] + face->normal.data[1];
  vertices[1 * 3 + 2] = face->points[1].data[2] + face->normal.data[2];
  vertices[2 * 3 + 0] = face->points[2].data[0] + face->normal.data[0];
  vertices[2 * 3 + 1] = face->points[2].data[1] + face->normal.data[1];
  vertices[2 * 3 + 2] = face->points[2].data[2] + face->normal.data[2];
  vertices[3 * 3 + 0] = face->points[0].data[0] + face->normal.data[0];
  vertices[3 * 3 + 1] = face->points[0].data[1] + face->normal.data[1];
  vertices[3 * 3 + 2] = face->points[0].data[2] + face->normal.data[2];

  draw_lines(vertices, 4, *color, thickness, pipeline);
}

static
void
populate_capsule_aabb(
  bvh_aabb_t* aabb, 
  const capsule_t* capsule, 
  const float multiplier)
{
  aabb->min_max[0] = capsule->center;
  aabb->min_max[1] = capsule->center;

  {
    vector3f min;
    vector3f_set_3f(
      &min,
      -capsule->radius * multiplier,
      (-capsule->half_height - capsule->radius) * multiplier,
      -capsule->radius * multiplier);
    add_set_v3f(aabb->min_max, &min);
    mult_set_v3f(&min, -1.f);
    add_set_v3f(aabb->min_max + 1, &min);
  }
}

static
void
populate_moving_capsule_aabb(
  bvh_aabb_t* aabb, 
  capsule_t capsule, 
  const vector3f* displacement, 
  const float multiplier)
{
  bvh_aabb_t start_end[2];
  populate_capsule_aabb(start_end + 0, &capsule, multiplier);
  add_set_v3f(&capsule.center, displacement);
  populate_capsule_aabb(start_end + 1, &capsule, multiplier);
  merge_aabb(aabb, start_end + 0, start_end + 1);
}

static
void
draw_text(
  pipeline_t* pipeline,
  font_runtime_t* font,
  const uint32_t font_image_id)
{
  {
    const char* text = "[3] RENDER COLLISION QUERIES";
    render_text_to_screen(
      font,
      font_image_id,
      pipeline,
      &text,
      1,
      draw_collision_query ? &red : &white,
      0.f, 120.f);
  }
  {
    const char* text = "[4] RENDER COLLISION FACE";
    render_text_to_screen(
      font,
      font_image_id,
      pipeline,
      &text,
      1,
      draw_collided_face ? &red : &white,
      0, 140.f);
  }
  {
    const char* text = "[5] SHOW SNAPPING/FALLING STATE";
    render_text_to_screen(
      font,
      font_image_id,
      pipeline,
      &text,
      1,
      draw_status ? &red : &white,
      0, 160.f);
  }
  {
    const char* text = "[9] SWITCH CAMERA MODE";
    render_text_to_screen(
      font,
      font_image_id,
      pipeline,
      &text,
      1,
      flying ? &red : &white,
      0, 180.f);
  }
}

static
uint32_t
is_in_filtered(
  uint32_t index, 
  const uint32_t to_filter[1024],
  uint32_t filter_count)
{
  for (uint32_t i = 0; i < filter_count; ++i) 
    if (to_filter[i] == index)
      return 1;
  return 0;
}

static
int32_t
intersects_post_displacement(
  capsule_t capsule,
  const vector3f displacement,
  const face_t* face,
  const vector3f* normal,
  const uint32_t iterations,
  const float limit_distance)
{
  vector3f penetration;
  point3f sphere_center;
  capsule_face_classification_t classification;

  add_set_v3f(&capsule.center, &displacement);
  classification = classify_capsule_face(
    &capsule, face, normal, 0, &penetration, &sphere_center);
  return classification != CAPSULE_FACE_NO_COLLISION;
}

static
intersection_info_t
get_first_time_of_impact_filtered(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  uint8_t only_floors,
  const uint32_t to_filter[1024],
  uint32_t filter_count,
  const uint32_t iterations,
  const float limit_distance,
  pipeline_t* pipeline)
{
  uint32_t array[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };

  populate_moving_capsule_aabb(&bounds, *capsule, &displacement, 1.025f);
  query_intersection(bvh, &bounds, array, &used);

  if (used && draw_collision_query) {
    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid)
          continue;

        draw_face(
          bvh->faces + i, 
          &bvh->faces[i].normal, 
          (bvh->faces[i].is_floor) ? &green : 
            (bvh->faces[i].is_ceiling ? &white : &red), 
          (bvh->faces[i].is_floor) ? 3 : 2,
          pipeline);
      }
    }
  }

  if (used) {
    face_t face;
    float time;

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid)
          continue;

        if (to_filter && is_in_filtered(i, to_filter, filter_count))
          continue;

        if (only_floors && !bvh->faces[i].is_floor)
          continue;

        if (!bounds_intersect(&bounds, &bvh->faces[i].aabb))
          continue;
        
        // TODO: we can avoid a copy here.
        face.points[0] = bvh->faces[i].points[0];
        face.points[1] = bvh->faces[i].points[1];
        face.points[2] = bvh->faces[i].points[2];

        // ignore any face that does not intersect post displacement.
        if (
          !intersects_post_displacement(
            *capsule, 
            displacement, 
            &face, 
            &bvh->faces[i].normal, 
            iterations, 
            limit_distance))
          continue;

        time = find_capsule_face_intersection_time(
          *capsule,
          &face,
          &bvh->faces[i].normal,
          displacement,
          iterations,
          limit_distance);

        if (time < info.time) {
          info.time = time;
          info.flags = 
            bvh->faces[i].is_floor ? COLLIDED_FLOOR_FLAG : info.flags;
          info.flags = 
            bvh->faces[i].is_ceiling ? COLLIDED_CEILING_FLAG : info.flags;
          info.flags = 
            info.flags == COLLIDED_NONE ? COLLIDED_WALLS : info.flags;
          info.bvh_face_index = i;
        }
      }
    }
  }

  return info;
}

static
intersection_info_t
get_first_time_of_impact(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  uint8_t only_floors,
  const uint32_t iterations,
  const float limit_distance,
  pipeline_t* pipeline)
{
  return 
    get_first_time_of_impact_filtered(
      bvh, 
      capsule, 
      displacement, 
      only_floors, 
      NULL, 
      0, 
      iterations, 
      limit_distance,
      pipeline);
}

static
int32_t
is_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t array[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  face_t face;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection(bvh, &bounds, array, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t* node = bvh->nodes + array[used_index];
    for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
      if (!bvh->faces[i].is_valid)
        continue;

      if (!bounds_intersect(&bounds, &bvh->faces[i].aabb))
        continue;

      face.points[0] = bvh->faces[i].points[0];
      face.points[1] = bvh->faces[i].points[1];
      face.points[2] = bvh->faces[i].points[2];

      classification =
        classify_capsule_face(
          capsule,
          &face,
          &bvh->faces[i].normal,
          0,
          &penetration,
          &sphere_center);

      length_sqrd = length_squared_v3f(&penetration);

      if (classification != CAPSULE_FACE_NO_COLLISION)
        return 0;
    }
  }

  return 1;
}

static
void
ensure_in_valid_space(
  bvh_t* bvh,
  capsule_t* capsule)
{
  vector3f penetration;
  point3f sphere_center;
  uint32_t array[256];
  uint32_t used = 0;
  bvh_aabb_t bounds;
  face_t face;
  capsule_face_classification_t classification;
  float length_sqrd;

  populate_capsule_aabb(&bounds, capsule, 1.025f);
  query_intersection(bvh, &bounds, array, &used);

  for (uint32_t used_index = 0; used_index < used; ++used_index) {
    bvh_node_t* node = bvh->nodes + array[used_index];
    for (uint32_t i = node->first_prim; i < node->last_prim; ++i) { 
      if (!bvh->faces[i].is_valid)
        continue;

      if (!bounds_intersect(&bounds, &bvh->faces[i].aabb))
        continue;

      // TODO(khalil): we can avoid the copy by providing an alternative to
      // the function or a cast the 3 float vecs to face_t.
      face.points[0] = bvh->faces[i].points[0];
      face.points[1] = bvh->faces[i].points[1];
      face.points[2] = bvh->faces[i].points[2];

      classification =
        classify_capsule_face(
          capsule,
          &face,
          &bvh->faces[i].normal,
          1,
          &penetration,
          &sphere_center);

      if (classification != CAPSULE_FACE_NO_COLLISION)
        add_set_v3f(&capsule->center, &penetration);
    }
  }
}

static
vector3f
get_adjusted_velocity(
  const vector3f velocity,
  const float toi, 
  const capsule_t* capsule,
  const float radius_scale)
{
  // if we are moving adn there is intersection.
  if (!IS_ZERO_LP(length_squared_v3f(&velocity)) && toi != 1.f) {
    vector3f resultant = normalize_v3f(&velocity);
    mult_v3f(&resultant, capsule->radius * radius_scale);
    add_set_v3f(&resultant, &velocity);
    return resultant;
  }

  return velocity;
}

static
int32_t
can_step_up(
  bvh_t* bvh,
  const capsule_t* capsule,
  const vector3f velocity,
  float* out_y,
  pipeline_t* pipeline)
{
  intersection_info_t info;
  vector3f displacement = { 0.f, -capsule->radius * 2, 0.f };
  capsule_t duplicate = *capsule;
  add_set_v3f(&duplicate.center, &velocity);

  // the capsule has to start in solid after applying the velocity.
  if (is_in_valid_space(bvh, &duplicate))
    return 0;

  {
    duplicate.center.data[1] -= displacement.data[1] / 2.f;
    info = get_first_time_of_impact(
      bvh, 
      &duplicate, 
      displacement, 1, 16, EPSILON_FLOAT_MIN_PRECISION, pipeline);

    if (info.flags == COLLIDED_FLOOR_FLAG) {
       float t;
       vector3f *normal = &bvh->faces[info.bvh_face_index].normal;
       face_t face;
       face.points[0] = bvh->faces[info.bvh_face_index].points[0];
       face.points[1] = bvh->faces[info.bvh_face_index].points[1];
       face.points[2] = bvh->faces[info.bvh_face_index].points[2];
       face = get_extended_face(&face, capsule->radius * 2);

       t = find_capsule_face_intersection_time(
        duplicate, 
        &face, 
        normal, 
        displacement, 
        16, 
        EPSILON_FLOAT_MIN_PRECISION);

       info.time = t;
       duplicate.center.data[1] += displacement.data[1] * info.time;
       
       if (is_in_valid_space(bvh, &duplicate)) {
        *out_y = duplicate.center.data[1];
        return 1;
       }
    }
  }

  return 0;
}

static
collision_flags_t
handle_collision_detection(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  const uint32_t iterations,
  const float limit_distance,
  pipeline_t* pipeline) 
{
  collision_flags_t flags = (collision_flags_t)0;
  intersection_info_t info, unfiltered_info;
  uint32_t to_filter[1024];
  uint32_t filter_count = 0;

  float length = length_v3f(&displacement);
  while (length > limit_distance) {
    info = get_first_time_of_impact_filtered(
      bvh, 
      capsule, 
      displacement, 
      0,
      to_filter, 
      filter_count, 
      iterations, 
      limit_distance,
      pipeline);

    unfiltered_info = get_first_time_of_impact(
      bvh,
      capsule,
      displacement,
      0,
      iterations,
      limit_distance,
      pipeline);

    if (info.flags == COLLIDED_NONE) {
      mult_set_v3f(&displacement, unfiltered_info.time);
      add_set_v3f(&capsule->center, &displacement);
      break;
    } else {
      vector3f normal = bvh->faces[info.bvh_face_index].normal;
      to_filter[filter_count++] = info.bvh_face_index;

      if (draw_collided_face)
        draw_face(
          bvh->faces + info.bvh_face_index, 
          &bvh->faces[info.bvh_face_index].normal, 
          &blue, 
          5, 
          pipeline);

      {
        vector3f scaled_displacement;
        float dot, toi = 1.f;
        dot = dot_product_v3f(&displacement, &normal);
        // ignore front facing or perpendicular faces.
        if (dot >= 0.f)
          continue;

        // due to imprecision, already considered faces can have smaller toi.
        toi = fmin(info.time, unfiltered_info.time);
        scaled_displacement = mult_v3f(&displacement, toi);
        add_set_v3f(&capsule->center, &scaled_displacement);

        // only keep the part of displacement that wasn't used.
        mult_set_v3f(&displacement, 1 - toi);

        {
          float out_y = 0.f;
          vector3f adjusted = get_adjusted_velocity(
            displacement, toi, capsule, 0.5f);
          // if we can step up, simply offset the capsule on y.
          if (can_step_up(bvh, capsule, adjusted, &out_y, pipeline))
            capsule->center.data[1] = out_y;
          else {
            vector3f scaled_normal = normal;
            dot = dot_product_v3f(&displacement, &normal);
            mult_set_v3f(&scaled_normal, dot);
            diff_set_v3f(&displacement, &scaled_normal);
          }
        }
      }

      flags |= info.flags;
      length = length_v3f(&displacement);
    }
  }

  return flags;
}

static
vector3f
handle_vertical_velocity(
  float delta_time, 
  vector3f velocity, 
  const vector3f velocity_limit,
  const float gravity_acceleration,
  const float jump_velocity, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline,
  font_runtime_t* font,
  const uint32_t font_image_id)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  uint32_t on_solid_floor = 0;
  intersection_info_t info;
  vector3f displacement = { 0.f, -capsule->radius * 2, 0.f };

  {
    // NOTE: this is not enough, the player could still be falling. the capsule
    // could simply be touching the side of the floor.
    capsule_t duplicate = *capsule;
    duplicate.center.data[1] -= displacement.data[1] / 2.f;
    info = get_first_time_of_impact(
      bvh, 
      &duplicate, 
      displacement, 1, 16, EPSILON_FLOAT_MIN_PRECISION, pipeline);

    if (info.flags == COLLIDED_FLOOR_FLAG) {
       float t;
       vector3f *normal = &bvh->faces[info.bvh_face_index].normal;
       face_t face;
       face.points[0] = bvh->faces[info.bvh_face_index].points[0];
       face.points[1] = bvh->faces[info.bvh_face_index].points[1];
       face.points[2] = bvh->faces[info.bvh_face_index].points[2];
       face = get_extended_face(&face, capsule->radius * 2);

       t = find_capsule_face_intersection_time(
        duplicate, 
        &face, 
        normal, 
        displacement, 
        16, 
        EPSILON_FLOAT_MIN_PRECISION);

       info.time = t;
       on_solid_floor = 1;
    }
  }

  // snap the capsule to the nearst floor.
  if (on_solid_floor && velocity.data[1] <= 0.f) {
    velocity.data[1] = 0.f;
    capsule->center.data[1] -= displacement.data[1] / 2.f;
    capsule->center.data[1] += displacement.data[1] * info.time;

    if (draw_status) {
      char text[512];
      const char* ptext = text;
      memset(text, 0, sizeof(text));
      sprintf(text, "SNAPPING %f", capsule->radius * info.time);
      render_text_to_screen(
        font,
        font_image_id,
        pipeline,
        &ptext,
        1,
        &green,
        400.f, 300.f);
    }
  }

  if (is_key_triggered(KEY_JUMP) && on_solid_floor) {
    on_solid_floor = 0;
    velocity.data[1] = jump_velocity;
  }

  if (!on_solid_floor) {
    velocity.data[1] -= gravity_acceleration * multiplier;
    velocity.data[1] = fmax(velocity.data[1], -velocity_limit.data[1]);
  }

  return velocity;
}

void
camera_update(
  float delta_time,
  camera_t* camera,
  bvh_t* bvh,
  pipeline_t* pipeline,
  font_runtime_t* font,
  const uint32_t font_image_id)
{
  static capsule_t capsule = { { 0.f, 0.f, 0.f }, 12.f, 16.f };

  capsule.center = camera->position;
  if (!is_in_valid_space(bvh, &capsule)) 
    ensure_in_valid_space(bvh, &capsule);
  
  handle_macro_keys(camera);
  cursor_info_t info = update_cursor(delta_time);
  camera_t oriented = get_camera_orientation(
    delta_time, &info, camera, 1/1000.f, 1/1000.f, TO_RADIANS(60));
  camera->lookat_direction = oriented.lookat_direction;
  camera->up_vector = oriented.up_vector;

  cam_speed = get_velocity(
    delta_time, 
    cam_speed, 
    friction_dec * friction_k, 
    cam_speed_limit, 
    cam_acc, 
    flying);

  if (!flying) {
    cam_speed = handle_vertical_velocity(
      delta_time, 
      cam_speed, 
      cam_speed_limit, 
      gravity_acc,
      jump_speed,
      bvh, 
      &capsule, 
      pipeline,
      font,
      font_image_id);
  }

  {
    vector3f displacement = get_relative_displacement(
      delta_time, &oriented, cam_speed);

    collision_flags_t flags = 
      handle_collision_detection(
        bvh, 
        &capsule, 
        displacement, 
        16, 
        EPSILON_FLOAT_MIN_PRECISION,
        pipeline);

    // reset the vertical velocity if we collide with a ceiling
    if (
      !flying &&
      (flags & COLLIDED_CEILING_FLAG) == COLLIDED_CEILING_FLAG &&
      cam_speed.data[1] > 0.f)
      cam_speed.data[1] = 0.f;

    // set the camera position to follow the capsule
    camera->position = capsule.center;
  }

  draw_text(pipeline, font, font_image_id);
}