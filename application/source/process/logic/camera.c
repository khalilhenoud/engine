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
#define KEY_DRAW_IGNORED_FACES    '6'
#define KEY_DISABLE_DEPTH         '7'
#define KEY_MOVEMENT_LOCK         '8'
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


static int32_t draw_ignored_faces = 0;
static int32_t disable_depth_debug = 0;
static int32_t use_locked_motion = 0;
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
static color_t gen = { 1.f, 1.f, 1.f, -1.f };

typedef
enum {
  COLLIDED_FLOOR_FLAG = 1 << 0,
  COLLIDED_CEILING_FLAG = 1 << 1,
  COLLIDED_WALLS_FLAG = 1 << 2,
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

typedef
struct {
  char text[512];
  color_t color;
  float x, y;
} text_properties_t;

typedef
struct {
  text_properties_t text_props[512];
  uint32_t used;
} text_renderables_t;

static text_renderables_t renderable_text; 

static
void
add_text_to_render(const char* text, color_t color, float x, float y)
{
  memset(
    renderable_text.text_props[renderable_text.used].text, 
    0, 
    sizeof(renderable_text.text_props[renderable_text.used].text));
  memcpy(
    renderable_text.text_props[renderable_text.used].text, 
    text, strlen(text));

  renderable_text.text_props[renderable_text.used].color = color;
  renderable_text.text_props[renderable_text.used].x = x;
  renderable_text.text_props[renderable_text.used].y = y;
  renderable_text.used++;
}

static
void
draw_renderable_text(
  pipeline_t* pipeline,
  font_runtime_t* font,
  const uint32_t font_image_id)
{
  const char* text;
  for (uint32_t i = 0; i < renderable_text.used; ++i) {
    text = renderable_text.text_props[i].text;
    render_text_to_screen(
      font, 
      font_image_id, 
      pipeline, 
      &text,
      1, 
      &renderable_text.text_props[i].color, 
      renderable_text.text_props[i].x, 
      renderable_text.text_props[i].y);
  }

  renderable_text.used = 0;
}

typedef
struct {
  uint32_t index;
  color_t color;
  int32_t thickness;
} renderable_face_properties_t;

typedef
struct {
  renderable_face_properties_t faces[2048];
  uint32_t used;
} face_renderables_t;

static face_renderables_t renderable_faces; 

static
void
draw_face(
  bvh_face_t* face, 
  vector3f* normal, 
  color_t* color, 
  int32_t thickness,
  pipeline_t* pipeline)
{
  const float mult = 0.1f;
  float vertices[12];

  vertices[0 * 3 + 0] = face->points[0].data[0] + face->normal.data[0] * mult;
  vertices[0 * 3 + 1] = face->points[0].data[1] + face->normal.data[1] * mult;
  vertices[0 * 3 + 2] = face->points[0].data[2] + face->normal.data[2] * mult;
  vertices[1 * 3 + 0] = face->points[1].data[0] + face->normal.data[0] * mult;
  vertices[1 * 3 + 1] = face->points[1].data[1] + face->normal.data[1] * mult;
  vertices[1 * 3 + 2] = face->points[1].data[2] + face->normal.data[2] * mult;
  vertices[2 * 3 + 0] = face->points[2].data[0] + face->normal.data[0] * mult;
  vertices[2 * 3 + 1] = face->points[2].data[1] + face->normal.data[1] * mult;
  vertices[2 * 3 + 2] = face->points[2].data[2] + face->normal.data[2] * mult;
  vertices[3 * 3 + 0] = face->points[0].data[0] + face->normal.data[0] * mult;
  vertices[3 * 3 + 1] = face->points[0].data[1] + face->normal.data[1] * mult;
  vertices[3 * 3 + 2] = face->points[0].data[2] + face->normal.data[2] * mult;

  if (disable_depth_debug) {
    disable_depth_test();
    draw_lines(vertices, 4, *color, thickness, pipeline);
    enable_depth_test();
  } else
    draw_lines(vertices, 4, *color, thickness, pipeline);
}

static
void
add_face_to_render(uint32_t index, color_t color, int32_t thickness)
{
  renderable_faces.faces[renderable_faces.used].index = index;
  renderable_faces.faces[renderable_faces.used].color = color;
  renderable_faces.faces[renderable_faces.used].thickness = thickness;
  renderable_faces.used++;
  renderable_faces.used %= 2048;
}

static
void
draw_renderable_faces(
  bvh_t* bvh,
  pipeline_t* pipeline)
{
  for (uint32_t i = 0; i < renderable_faces.used; ++i) {
    uint32_t index = renderable_faces.faces[i].index;
    color_t normal_color;
    uint32_t gen_color = 0;
    if (renderable_faces.faces[i].color.data[3] == gen.data[3]) {
      normal_color.data[0] = bvh->faces[index].normal.data[0] + 0.3f;
      normal_color.data[1] = bvh->faces[index].normal.data[1] + 0.3f;
      normal_color.data[2] = bvh->faces[index].normal.data[2] + 0.3f;
      normal_color.data[3] = 1.f;
      gen_color = 1;
    }

    draw_face(
      bvh->faces + index, 
      &bvh->faces[index].normal, 
      gen_color ? &normal_color: &renderable_faces.faces[i].color, 
      renderable_faces.faces[i].thickness,
      pipeline);
  }

  renderable_faces.used = 0;
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

  if (use_locked_motion)
    return current_velocity;

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

  if (is_key_triggered(KEY_DRAW_IGNORED_FACES))
    draw_ignored_faces = !draw_ignored_faces;

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

  if (is_key_triggered(KEY_MOVEMENT_LOCK))
    use_locked_motion = !use_locked_motion;

  if (is_key_triggered(KEY_DISABLE_DEPTH))
    disable_depth_debug = !disable_depth_debug;
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
uint32_t
get_all_first_time_of_impact_filtered(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  intersection_info_t collision_info[256],
  const uint32_t to_filter[1024],
  uint32_t filter_count,
  const uint32_t iterations,
  const float limit_distance)
{
  uint32_t array[256];
  uint32_t used = 0;
  uint32_t collision_info_used = 0;
  bvh_aabb_t bounds;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };

  // initialize the first element.
  collision_info[0] = info;

  populate_moving_capsule_aabb(&bounds, *capsule, &displacement, 1.025f);
  query_intersection(bvh, &bounds, array, &used);

  if (used && draw_collision_query) {
    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid)
          continue;

        {
          color_t color = 
            (bvh->faces[i].is_floor) ? green : 
            (bvh->faces[i].is_ceiling ? white : gen);
          int32_t thickness = (bvh->faces[i].is_floor) ? 3 : 2;
          add_face_to_render(i, color, thickness);
        }
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

        if (
          time < collision_info[0].time || 
          IS_SAME_MP(time, collision_info[0].time)) {
          collision_info_used = 
            time < collision_info[0].time ? 0 : collision_info_used;
            
          collision_info[collision_info_used].time = time;
          collision_info[collision_info_used].flags = COLLIDED_NONE;

          collision_info[collision_info_used].flags = 
            bvh->faces[i].is_floor ? 
            COLLIDED_FLOOR_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].flags = 
            bvh->faces[i].is_ceiling ? 
            COLLIDED_CEILING_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].flags = 
            collision_info[collision_info_used].flags == COLLIDED_NONE ? 
            COLLIDED_WALLS_FLAG : collision_info[collision_info_used].flags;

          collision_info[collision_info_used].bvh_face_index = i;
          collision_info_used++;
          assert(collision_info_used < 256);
        }
      }
    }
  }

  return collision_info_used;
}

static
uint32_t
get_all_first_time_of_impact(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  intersection_info_t collision_info[256],
  const uint32_t iterations,
  const float limit_distance)
{
  return 
    get_all_first_time_of_impact_filtered(
      bvh, 
      capsule, 
      displacement, 
      collision_info,
      NULL, 
      0, 
      iterations, 
      limit_distance);
}

static
intersection_info_t
get_any_first_time_of_impact_filtered(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  const uint32_t to_filter[1024],
  uint32_t filter_count,
  const uint32_t iterations,
  const float limit_distance)
{
  uint32_t info_used = 0;
  intersection_info_t collision_info[256];
  collision_info[0].time = 1.f;
  collision_info[0].flags = COLLIDED_NONE;
  collision_info[0].bvh_face_index = (uint32_t)-1;

  get_all_first_time_of_impact_filtered(
    bvh, 
    capsule, 
    displacement, 
    collision_info,
    to_filter,
    filter_count,
    iterations, 
    limit_distance);

  return collision_info[0];
}

static
intersection_info_t
get_any_first_time_of_impact(
  bvh_t* bvh,
  capsule_t* capsule,
  vector3f displacement,
  const uint32_t iterations,
  const float limit_distance)
{
  return 
    get_any_first_time_of_impact_filtered(
      bvh, 
      capsule, 
      displacement,
      NULL, 
      0, 
      iterations, 
      limit_distance);
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

      if (
        classification != CAPSULE_FACE_NO_COLLISION && 
        !IS_ZERO_LP(length_sqrd))
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
          0,
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
  // if we are moving and there is intersection.
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
  vector3f velocity,
  const float toi,
  float radius_scale,
  float* out_y)
{
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

    info_used = get_all_first_time_of_impact(
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
       vector3f *normal = &bvh->faces[info.bvh_face_index].normal;
       face_t face;
       face.points[0] = bvh->faces[info.bvh_face_index].points[0];
       face.points[1] = bvh->faces[info.bvh_face_index].points[1];
       face.points[2] = bvh->faces[info.bvh_face_index].points[2];
       face = get_extended_face(&face, capsule->radius * 2);

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
       duplicate.center.data[1] += displacement.data[1] * info.time;
       
       *out_y = duplicate.center.data[1];
       return 1;
    }
  }

  return 0;
}

// return 0 if not split, 1 if split.
static
int32_t
classify_buckets(
  bvh_t* bvh, 
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256],
  uint32_t bucket_count, 
  collision_flags_t flag,
  uint32_t bucket_index,
  uint32_t excluded_index)
{
  face_t face0, face1;
  vector3f* normal0;
  uint32_t index0, index1;
  point_halfspace_classification_t classify[3];
  uint32_t faces_in_front, faces_to_back;

  assert(bucket_count);
  
  for (uint32_t i = 0; i < bucket_count; ++i) {
    // these are the buckets that form colinear opposite walls
    if (i == bucket_index || i == excluded_index)
      continue;

    // again limit this to similar specs
    if ((collision_info[bucket_offset[i]].flags & flag) == 0)
      continue;

    // this is the plane, test the original bucket faces to check if they lie on 
    // both sides of the plane. 
    index0 = collision_info[bucket_offset[i]].bvh_face_index;
    memcpy(face0.points, bvh->faces[index0].points, sizeof(face0.points));
    normal0 = &bvh->faces[index0].normal;
    faces_in_front = faces_to_back = 0;

    for (uint32_t j = 0; j < buckets[bucket_index]; ++j) {
      uint32_t points_in_front = 0, points_to_back = 0;
      index1 = collision_info[bucket_offset[bucket_index] + j].bvh_face_index;
      memcpy(face1.points, bvh->faces[index1].points, sizeof(face1.points));

      classify[0] = classify_point_halfspace(&face0, normal0, face1.points + 0);
      classify[1] = classify_point_halfspace(&face0, normal0, face1.points + 1);
      classify[2] = classify_point_halfspace(&face0, normal0, face1.points + 2);
      
      points_in_front += (classify[0] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[1] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      points_in_front += (classify[2] == POINT_IN_POSITIVE_HALFSPACE) ? 1 : 0;
      
      points_to_back += (classify[0] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[1] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      points_to_back += (classify[2] == POINT_IN_NEGATIVE_HALFSPACE) ? 1 : 0;
      
      // early out, meaning we have found a face that intersects the clip plane.
      if (points_in_front && points_to_back)
        return 1;
      else if (points_to_back) 
        faces_to_back++;
      else
        faces_in_front++;

      // early out if the classification contradicts the one-sidedness 
      // established prior.
      if (faces_in_front && faces_to_back)
        return 1;
    }
  }

  return 0;
}

// NOTE: the function modifies buckets[], bucket_offset[].
// TODO: make this more robust, it should also modify bucket_count and tag with
// the appropriate access specfiers to indicate intent (const etc...)
static
uint32_t
remove_bucket(
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256], 
  uint32_t bucket_offset[256], 
  uint32_t bucket_count,
  uint32_t bucket_index)
{
  // remove all the faces belonging to the bucket, remove the bucket and rebuild
  // the offset.
  uint32_t start_index = bucket_offset[bucket_index];
  uint32_t end_index = start_index + buckets[bucket_index];
  uint32_t counts_affected = buckets[bucket_index];

  intersection_info_t new_info[256];
  uint32_t new_buckets[256];
  uint32_t new_bucket_offset[256];
  uint32_t new_bucket_count = bucket_count - 1;

  assert(bucket_count);

  for (uint32_t i = 0, index = 0; i < bucket_count; ++i) {
    if (i == bucket_index)
      continue;
    new_buckets[index++] = buckets[i];
  }

  for (uint32_t i = 0, index = 0; i < new_bucket_count; ++i) {
    new_bucket_offset[i] = index;
    index += new_buckets[i];
  }

  for (uint32_t i = 0, index = 0; i < info_used; ++i) {
    if (i >= start_index && i < end_index) {
      if (draw_ignored_faces)
        add_face_to_render(collision_info[i].bvh_face_index, yellow, 2);
      continue;
    }
    new_info[index++] = collision_info[i];
  }

  memcpy(buckets, new_buckets, sizeof(new_buckets));
  memcpy(bucket_offset, new_bucket_offset, sizeof(new_bucket_offset));
  memcpy(collision_info, new_info, sizeof(new_info));

  return info_used - counts_affected;
}

static
int32_t
process_buckets(
  bvh_t* const bvh, 
  intersection_info_t collision_info[256], 
  uint32_t info_used, 
  uint32_t buckets[256],
  uint32_t bucket_count)
{
  // processing is separated based on walls, floors, ceilings
  collision_flags_t flags[] = { 
    COLLIDED_WALLS_FLAG, 
    COLLIDED_FLOOR_FLAG | COLLIDED_CEILING_FLAG};

  // transform to total offset, instead of count.
  uint32_t bucket_offset[256] = { 0 };
  for (uint32_t i = 0, index = 0; i < bucket_count; ++i) {
    bucket_offset[i] = index;
    index += buckets[i];
  }

  assert(bucket_count);

  for (uint32_t flag_index = 0; flag_index < 2; ++flag_index) {
    planes_classification_t result;
    int32_t parters[] = { -1, -1 };
    int32_t found = 1;
    face_t face0, face1;
    vector3f* normal0, * normal1;
    uint32_t index0, index1;
    collision_flags_t flag = flags[flag_index];

    // while colinear opposite face buckets exist, continue.
    while (found) {
      found = 0;

      for (uint32_t i = 0, count = bucket_count - 1; i < count; ++i) {
        // only consider buckets that share the flag spec
        if ((collision_info[bucket_offset[i]].flags & flag) == 0)
          continue;
        
        index0 = collision_info[bucket_offset[i]].bvh_face_index;
        memcpy(face0.points, bvh->faces[index0].points, sizeof(face0.points));
        normal0 = &bvh->faces[index0].normal;

        for (uint32_t j = i + 1; j < bucket_count; ++j) {
          // again limit it to the same spec
          if ((collision_info[bucket_offset[j]].flags & flag) == 0)
            continue;

          index1 = collision_info[bucket_offset[j]].bvh_face_index;
          memcpy(face1.points, bvh->faces[index1].points, sizeof(face1.points));
          normal1 = &bvh->faces[index1].normal;

          result = classify_planes(&face0, normal0, &face1, normal1);
          if (result == PLANES_COLINEAR_OPPOSITE_FACING) {
            parters[0] = i;
            parters[1] = j; 
            found = 1;
            break;
          }
        }

        if (found)
          break;
      }

      if (found) {
        // one or both of the parters buckets will be removed.
        if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[0], parters[1])) {
          // remove parters[1]
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[1]);
          --bucket_count;
        } else if (classify_buckets(
          bvh, 
          collision_info, info_used, 
          buckets, bucket_offset, bucket_count, 
          flag, 
          parters[1], parters[0])) {
          // remove parters[0]
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[0]);
          --bucket_count;
        } else {
          // remove parters[0, 1].
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[0]);
          --bucket_count;
          --parters[1];
          info_used = remove_bucket(
            collision_info, info_used, 
            buckets, bucket_offset, bucket_count, 
            parters[1]);
          --bucket_count;
        }
      }
    }
  }

  return info_used;
}

// this will sort collision_info with colinear faces being consecutive, the
// buckets array will contain the count per face colinearity. we return the
// bucket type.
static
uint32_t
sort_in_buckets(
  bvh_t* const bvh,
  intersection_info_t collision_info[256], 
  uint32_t info_used,
  uint32_t buckets[256])
{
  uint32_t bucket_count = 0;
  intersection_info_t sorted[256];
  uint32_t sorted_index = 0;
  uint32_t copy_info_used = info_used;
  bvh_face_t* a_bvh_face, *b_bvh_face;
  face_t a, b;
  vector3f* a_normal, *b_normal;
  int64_t i, to_swap;
  intersection_info_t copy;
  planes_classification_t classify;

  while (info_used--) {
    a_bvh_face = &bvh->faces[collision_info[info_used].bvh_face_index]; 
    memcpy(a.points, a_bvh_face->points, sizeof(a.points));
    a_normal = &a_bvh_face->normal;
    
    // copy into the first sorted index.
    sorted[sorted_index++] = collision_info[info_used];
    buckets[bucket_count++] = 1;

    i = to_swap = (int64_t)info_used - 1;
    for (; i >= 0; --i) {
      b_bvh_face = &bvh->faces[collision_info[i].bvh_face_index];
      memcpy(b.points, b_bvh_face->points, sizeof(b.points));
      b_normal = &b_bvh_face->normal;

      // if colinear copy into the first sorted index, swap it with to_swap 
      // index and decrease info_used.
      classify = classify_planes(&a, a_normal, &b, b_normal);
      if (classify == PLANES_COLINEAR) {
        sorted[sorted_index++] = collision_info[i];
        buckets[bucket_count - 1]++;

        // swap the colinear with the non-colinear.
        copy = collision_info[to_swap];
        collision_info[to_swap] = collision_info[i];
        collision_info[i] = copy;

        to_swap--;
        info_used--;
      }
    }
  }

  assert(sorted_index == copy_info_used);
  
  // copy the sorted data back.
  memcpy(collision_info, sorted, sizeof(sorted));

  return bucket_count;
}

static
vector3f
get_averaged_normal(
  bvh_t* const bvh,
  const uint32_t on_solid_floor,
  intersection_info_t collision_info[256],
  uint32_t info_used,
  collision_flags_t* flags)
{
  uint32_t buckets[256];
  uint32_t bucket_count = 0;

  vector3f averaged, normal;
  vector3f_set_1f(&averaged, 0.f);

  // we shouldn't consider colinear faces more than once, we need to sort this.
  bucket_count = sort_in_buckets(bvh, collision_info, info_used, buckets);

  for (uint32_t i = 0, index = 0, face_i, is_wall; i < bucket_count; ++i) {
    is_wall = 0;
    face_i = collision_info[index].bvh_face_index;
    normal = bvh->faces[face_i].normal;

    if (bvh->faces[face_i].is_floor)
      *flags |= COLLIDED_FLOOR_FLAG;
    else if (bvh->faces[face_i].is_ceiling)
      *flags |= COLLIDED_CEILING_FLAG;
    else {
      *flags |= COLLIDED_WALLS_FLAG;
      is_wall = 1;
    }

    // adjust the normal, even if sloped, walls cannot contribute to vertical
    // acceleration.
    if (is_wall && on_solid_floor) {
      vector3f y_up, perp;
      vector3f_set_3f(&y_up, 0.f, 1.f, 0.f);
      perp = cross_product_v3f(&y_up, &normal);
      normalize_set_v3f(&perp);
      normal = cross_product_v3f(&perp, &y_up);
      normalize_set_v3f(&normal);
    }

    add_set_v3f(&averaged, &normal);

     if (draw_collided_face) {
       for (uint32_t k = index; k < (index + buckets[i]); ++k) {
         uint32_t d_face_i = collision_info[k].bvh_face_index;
         color_t color =
           (bvh->faces[d_face_i].is_floor) ? green :
           (bvh->faces[d_face_i].is_ceiling ? white : gen);
         add_face_to_render(d_face_i, color, 2);
       }
     }

    index += buckets[i];
  }

  normalize_set_v3f(&averaged);
  return averaged;
}

static
uint32_t
trim_backfacing(
  bvh_t* const bvh,
  const vector3f* velocity, 
  intersection_info_t collision_info[256],
  uint32_t info_used)
{
  intersection_info_t new_collision_info[256];
  uint32_t count = 0;
  vector3f* normal = NULL;

  for (uint32_t i = 0, index = 0; i < info_used; ++i) {
    index = collision_info[i].bvh_face_index;
    normal = &bvh->faces[index].normal;
    if (dot_product_v3f(velocity, normal) > EPSILON_FLOAT_LOW_PRECISION)
      continue;
    else
      new_collision_info[count++] = collision_info[i];
  }

  // copy the new info into the old one and return the updated info_used
  memcpy(collision_info, new_collision_info, sizeof(new_collision_info));
  return count;
}

// Note: this function does modify collision_info.
static
int32_t 
process_collision_info(
  bvh_t* const bvh, 
  const vector3f* velocity, 
  intersection_info_t collision_info[256], 
  int32_t info_used)
{
  // sort and process the buckets in a way the collision info used.
  if (info_used) {
    uint32_t buckets[256];
    uint32_t bucket_count = 0;
    bucket_count = sort_in_buckets(bvh, collision_info, info_used, buckets);
    info_used = process_buckets(
      bvh, collision_info, info_used, buckets, bucket_count);
  }

  return trim_backfacing(bvh, velocity, collision_info, info_used);
}

static
collision_flags_t
handle_collision_detection(
  bvh_t* bvh,
  capsule_t* capsule,
  const vector3f displacement,
  const uint32_t on_solid_floor,
  const uint32_t iterations,
  const float limit_distance)
{
  collision_flags_t flags = (collision_flags_t)0;
  intersection_info_t collision_info[256];
  uint32_t info_used;
  vector3f orientation = normalize_v3f(&displacement);
  vector3f velocity = displacement;
  float distance_to_go = length_v3f(&velocity);
  // limit the number of iterations we can do on the collision handling.
  int32_t steps = 3;

  while (steps-- && !IS_ZERO_LP(length_squared_v3f(&velocity))) {
    info_used = get_all_first_time_of_impact(
      bvh, 
      capsule, 
      velocity,
      collision_info,  
      iterations, 
      limit_distance);

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
        bvh, on_solid_floor, collision_info, info_used, &l_flags);
      flags |= l_flags;

      to_apply = mult_v3f(&velocity, toi);
      add_set_v3f(&capsule->center, &to_apply);
      distance_to_go -= length_v3f(&to_apply);
      distance_to_go = fmax(distance_to_go, 0.f);

      if (
        (l_flags & COLLIDED_WALLS_FLAG) && 
        can_step_up(bvh, capsule, velocity, toi, 0.5f, &step_y)) {
        
        if (draw_status) {
          float value = capsule->center.data[1] - step_y;
          char text[512];
          memset(text, 0, sizeof(text));
          sprintf(text, "STEPUP %f", value);
          add_text_to_render(text, red, 400.f, 320.f);
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
vector3f
handle_vertical_velocity(
  float delta_time, 
  vector3f velocity, 
  uint32_t* on_solid_floor,
  const uint32_t iterations,
  const float limit_distance,
  const vector3f velocity_limit,
  const float gravity_acceleration,
  const float jump_velocity, 
  bvh_t* bvh, 
  capsule_t* capsule)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  intersection_info_t collision_info[256];
  uint32_t info_used;
  int32_t floor_info_i;
  intersection_info_t info = { 1.f, COLLIDED_NONE, (uint32_t)-1 };
  vector3f displacement = { 0.f, 0.f, 0.f };
  const float y_trace_start_offset = capsule->radius;
  const float y_trace_end_offset = -capsule->radius * 2;
  // this is necessary as the time of first impact precision is 1 / iterations.
  displacement.data[1] = y_trace_end_offset + y_trace_end_offset / iterations;
  *on_solid_floor = 0;

  {
    capsule_t duplicate = *capsule;
    duplicate.center.data[1] += y_trace_start_offset;

    info_used = get_all_first_time_of_impact(
      bvh, 
      &duplicate, 
      displacement, 
      collision_info, 
      iterations, 
      limit_distance);

    floor_info_i = get_floor_info(collision_info, info_used);
    if (floor_info_i != -1)
      info = collision_info[floor_info_i];

    if (info.flags == COLLIDED_FLOOR_FLAG) {
      float t;
      vector3f* normal = &bvh->faces[info.bvh_face_index].normal;
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
        iterations,
        limit_distance);

      info.time = t;
      *on_solid_floor = 1;
    }
  }

  // snap the capsule to the nearst floor.
  if (*on_solid_floor && velocity.data[1] <= 0.f) {
    velocity.data[1] = 0.f;
    capsule->center.data[1] += y_trace_start_offset;
    capsule->center.data[1] += displacement.data[1] * info.time;

    if (draw_status) {
      float distance = 
        displacement.data[1] * info.time + y_trace_start_offset;
      char text[512];
      memset(text, 0, sizeof(text));
      sprintf(text, "SNAPPING %f", distance);
      add_text_to_render(text, green, 400.f, 300.f);

      {
        uint32_t i = info.bvh_face_index;
        color_t color = 
          (bvh->faces[i].is_floor) ? green : 
          (bvh->faces[i].is_ceiling ? white : gen);
        add_face_to_render(i, color, 1);
      }
    }
  }

  if (is_key_triggered(KEY_JUMP) && *on_solid_floor) {
    *on_solid_floor = 0;
    velocity.data[1] = jump_velocity;
  }

  if (!*on_solid_floor) {
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
  static uint32_t on_solid_floor = 0;

  // TODO: cap the detla time when debugging.
  delta_time = fmin(delta_time, REFERENCE_FRAME_TIME);

  handle_macro_keys(camera);

  capsule.center = camera->position;
  // TODO: this should be called from level1.c, remove it.
  if (!is_in_valid_space(bvh, &capsule)) 
    ensure_in_valid_space(bvh, &capsule);
  
  cursor_info_t info = update_cursor(delta_time);
  camera_t oriented = get_camera_orientation(
    delta_time, &info, camera, 1/1000.f, 1/1000.f, TO_RADIANS(60));
  if (use_locked_motion)
    oriented = *camera;
  else {
    camera->lookat_direction = oriented.lookat_direction;
    camera->up_vector = oriented.up_vector;
  }

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
      &on_solid_floor,
      16, 
      EPSILON_FLOAT_MIN_PRECISION,
      cam_speed_limit, 
      gravity_acc,
      jump_speed,
      bvh, 
      &capsule);
  }

  {
    vector3f displacement = get_relative_displacement(
      delta_time, &oriented, cam_speed);

    collision_flags_t flags = 
      handle_collision_detection(
        bvh, 
        &capsule, 
        displacement, 
        on_solid_floor,
        16, 
        EPSILON_FLOAT_MIN_PRECISION);

    // reset the vertical velocity if we collide with a ceiling
    if (
      !flying &&
      (flags & COLLIDED_CEILING_FLAG) == COLLIDED_CEILING_FLAG &&
      cam_speed.data[1] > 0.f)
      cam_speed.data[1] = 0.f;

    // set the camera position to follow the capsule
    camera->position = capsule.center;
  }

  {
    float y = 100.f;
    char array[256];
    snprintf(
      array, 256, 
      "CAPSULE POSITION:     %.2f     %.2f     %.2f", 
      capsule.center.data[0], 
      capsule.center.data[1], 
      capsule.center.data[2]);

    add_text_to_render(
      array, green, 0.f, (y+=20.f));
    add_text_to_render(
      "[3] RENDER COLLISION QUERIES", 
      draw_collision_query ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[4] RENDER COLLISION FACE", 
      draw_collided_face ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[5] SHOW SNAPPING/FALLING STATE", 
      draw_status ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[6] DRAW IGNORED FACES", 
      draw_ignored_faces ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[7] DISABLE DEPTH TEST DEBUG", 
      disable_depth_debug ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[8] LOCK DIRECTIONAL MOTION", 
      use_locked_motion ? red : white, 0.f, (y+=20.f));
    add_text_to_render(
      "[9] SWITCH CAMERA MODE", flying ? red : white, 0.f, (y+=20.f));
    draw_renderable_text(pipeline, font, font_image_id);
    draw_renderable_faces(bvh, pipeline);
  }
}