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


static int32_t prev_mouse_x = -1; 
static int32_t prev_mouse_y = -1;
static int32_t draw_collision_query = 0;
static int32_t draw_collided_face = 0;
static int32_t draw_status = 0;
static vector3f cam_acc = { 5.f, 5.f, 5.f };
static vector3f cam_speed = { 0.f, 0.f, 0.f };
static vector3f cam_speed_limit = { 10.f, 10.f, 10.f };
static const float gravity_acc = 1.f;
static const float friction_dec = 2.f;
static const float friction_k = 1.f;
static const float jump_speed = 20.f;

// 0 is walking, 1 is flying (not yet used).
static uint32_t movement_mode = 0;

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

void
recenter_camera_cursor(void)
{
  prev_mouse_x = -1;
  prev_mouse_y = -1;
}

static
void
handle_input(float delta_time, camera_t* camera)
{
  static float y_limit = 0.f;
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  
  matrix4f crossup, camerarotateY, camerarotatetemp, tmpmatrix;
  float dx, dy, d, tempangle;
  int32_t mouse_x, mouse_y = 0;
  
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

  if (is_key_triggered(KEY_MOVEMENT_MODE))
    movement_mode = !movement_mode;

  // Handling translations.
  if (movement_mode) {
    if (is_key_pressed(KEY_MOVE_DOWN))
      cam_speed.data[1] -= cam_acc.data[1] * multiplier;

    if (is_key_pressed(KEY_MOVE_UP))
      cam_speed.data[1] += cam_acc.data[1] * multiplier;
  }

  if (is_key_pressed(KEY_STRAFE_LEFT))
    cam_speed.data[0] -= cam_acc.data[0] * multiplier;

  if (is_key_pressed(KEY_STRAFE_RIGHT))
    cam_speed.data[0] += cam_acc.data[0] * multiplier;

  if (is_key_pressed(KEY_MOVE_FWD))
    cam_speed.data[2] += cam_acc.data[2] * multiplier;

  if (is_key_pressed(KEY_MOVE_BACK))
    cam_speed.data[2] -= cam_acc.data[2];

  {
    {
      // apply friction.
      float friction = friction_k * friction_dec * multiplier;
      if (cam_speed.data[0] < 0.f) {
        cam_speed.data[0] += friction;
        cam_speed.data[0] = fmin(cam_speed.data[0], 0.f);
      } else {
        cam_speed.data[0] -= friction;
        cam_speed.data[0] = fmax(cam_speed.data[0], 0.f);
      }

      if (cam_speed.data[2] < 0.f) {
        cam_speed.data[2] += friction;
        cam_speed.data[2] = fmin(cam_speed.data[2], 0.f);
      } else {
        cam_speed.data[2] -= friction;
        cam_speed.data[2] = fmax(cam_speed.data[2], 0.f);
      }

      if (movement_mode) {
        if (cam_speed.data[1] < 0.f) {
          cam_speed.data[1] += friction;
          cam_speed.data[1] = fmin(cam_speed.data[1], 0.f);
        }
        else {
          cam_speed.data[1] -= friction;
          cam_speed.data[1] = fmax(cam_speed.data[1], 0.f);
        }
      }
    }

    {
      // limit cam_speed
      cam_speed.data[0] = (cam_speed.data[0] < -cam_speed_limit.data[0]) ? 
        -cam_speed_limit.data[0] : cam_speed.data[0];
      cam_speed.data[0] = (cam_speed.data[0] > +cam_speed_limit.data[0]) ? 
        +cam_speed_limit.data[0] : cam_speed.data[0];
      cam_speed.data[2] = (cam_speed.data[2] < -cam_speed_limit.data[2]) ? 
        -cam_speed_limit.data[2] : cam_speed.data[2];
      cam_speed.data[2] = (cam_speed.data[2] > +cam_speed_limit.data[2]) ? 
        +cam_speed_limit.data[2] : cam_speed.data[2];
      if (movement_mode) {
        cam_speed.data[1] = (cam_speed.data[1] < -cam_speed_limit.data[1]) ?
          -cam_speed_limit.data[1] : cam_speed.data[1];
        cam_speed.data[1] = (cam_speed.data[1] > +cam_speed_limit.data[1]) ?
          +cam_speed_limit.data[1] : cam_speed.data[1];
      }
    }
  }

  {
    vector3f cam_dt_pos = { 0.f, 0.f, 0.f };
    cam_dt_pos.data[0] += tmp.data[0] * cam_speed.data[0] * multiplier;
    cam_dt_pos.data[2] += tmp.data[2] * cam_speed.data[0] * multiplier;
    if (movement_mode)
      cam_dt_pos.data[1] += cam_speed.data[1] * multiplier;

    {
      vector3f vecxz;
      vecxz.data[0] = camera->lookat_direction.data[0];
      vecxz.data[2] = camera->lookat_direction.data[2];
      length = 
        sqrtf(vecxz.data[0] * vecxz.data[0] + vecxz.data[2] * vecxz.data[2]);
      if (length != 0) {
        vecxz.data[0] /= length;
        vecxz.data[2] /= length;

        cam_dt_pos.data[0] += vecxz.data[0] * cam_speed.data[2] * multiplier;
        cam_dt_pos.data[2] += vecxz.data[2] * cam_speed.data[2] * multiplier;
      }
    }

    camera->position.data[0] += cam_dt_pos.data[0];
    camera->position.data[2] += cam_dt_pos.data[2];

    if (movement_mode)
      camera->position.data[1] += cam_dt_pos.data[1];
  }
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
populate_capsule_aabb(bvh_aabb_t* aabb, const capsule_t* capsule)
{
  aabb->min_max[0] = capsule->center;
  aabb->min_max[1] = capsule->center;

  {
    vector3f min;
    vector3f_set_3f(
      &min,
      -capsule->radius,
      (-capsule->half_height - capsule->radius),
      -capsule->radius);
    add_set_v3f(aabb->min_max, &min);
    mult_set_v3f(&min, -1.f);
    add_set_v3f(aabb->min_max + 1, &min);
  }
}

// TODO: move this function to the collision packag.
static
float
find_intersection_time(
  sphere_t sphere, 
  face_t* face,
  vector3f* normal,
  const float expand_down)
{
  const uint32_t iteration_max = 16;
  const float distance_limit = 1.f;
  float starting_y = sphere.center.data[1];

  {
    float start = 0.f, end = 1.f, mid_point;
    uint32_t iteration = 0;
    vector3f penetration;
    sphere_face_classification_t classification = classify_sphere_face(
      &sphere, face, normal, 0, &penetration);

    assert(
      classification == SPHERE_FACE_NO_COLLISION && 
      "the initial test should always return no collision!");

    {
      float distance = get_sphere_face_distance(&sphere, face, normal);

      do {
        mid_point = start + (end - start) / 2.f;
        sphere.center.data[1] = starting_y - mid_point * expand_down;
        classification = classify_sphere_face(
          &sphere, face, normal, 0, &penetration);
        if (classification == SPHERE_FACE_NO_COLLISION) {
          start = mid_point;
          distance = get_sphere_face_distance(&sphere, face, normal);
        }
        else
          end = mid_point;
      } while (distance > distance_limit && ++iteration < iteration_max);
    }

    return start;
  }
}

// NOTE: The initial position of the capsule should not intersect any floor.
// This condition must be ensured by the calling function.
static
uint32_t
snaps_to_floor_at(
  const capsule_t* capsule, 
  bvh_t* bvh,
  const float expand_down,
  float* distance)
{
  capsule_t swept_sphere;
  bvh_aabb_t swept_aabb;
  uint32_t array[256];
  uint32_t used = 0;
  // to simplify the calculations we limit the faces to 256 total.
  uint32_t faces[256];
  uint32_t faces_total = 0;

  // intersect the aabb to limit our calculations to the faces we care about.
  bvh_aabb_t bounds;
  vector3f min, max, sphere_center;
  vector3f_set_3f(
    &sphere_center, 
    capsule->center.data[0], 
    capsule->center.data[1] - capsule->half_height, 
    capsule->center.data[2]);
  bounds.min_max[0] = bounds.min_max[1] = sphere_center;
  vector3f_set_3f(
    &min, 
    -capsule->radius, 
    -capsule->radius - expand_down, 
    -capsule->radius);
  vector3f_set_3f(
    &max,
    capsule->radius, 
    capsule->radius, 
    capsule->radius);
  add_set_v3f(bounds.min_max, &min);
  add_set_v3f(bounds.min_max + 1, &max);

  query_intersection(bvh, &bounds, array, &used);

  // this swept sphere will be used to limit the faces we look at.
  swept_sphere.radius = capsule->radius;
  swept_sphere.center = sphere_center;
  swept_sphere.center.data[1] -= expand_down/2.f;
  swept_sphere.half_height = expand_down/2.f;
  populate_capsule_aabb(&swept_aabb, &swept_sphere);

  if (used) {
    face_t face;
    vector3f penetration;
    capsule_face_classification_t classification;
    point3f center;

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid || !bvh->faces[i].is_floor)
          continue;

        if (!bounds_intersect(&swept_aabb, &bvh->faces[i].aabb))
          continue;
        
        // TODO: we can avoid a copy here.
        face.points[0] = bvh->faces[i].points[0];
        face.points[1] = bvh->faces[i].points[1];
        face.points[2] = bvh->faces[i].points[2];

        classification = classify_capsule_face(
          &swept_sphere, 
          &face, 
          &bvh->faces[i].normal, 
          0, 
          &penetration, 
          &center);

        if (classification != CAPSULE_FACE_NO_COLLISION) {
          faces[faces_total++] = i;
          assert(faces_total < 256 && "no more than 256 faces are supported!");
        }
      }
    }
  }

  if (!faces_total)
    return 0;

  {
    float t = 1.f, tmp = 1.f;
    face_t face;
    sphere_t sphere;
    sphere.center = sphere_center;
    sphere.radius = capsule->radius;

    for (uint32_t i = 0; i < faces_total; ++i) {
      face.points[0] = bvh->faces[faces[i]].points[0];
      face.points[1] = bvh->faces[faces[i]].points[1];
      face.points[2] = bvh->faces[faces[i]].points[2];
      tmp = find_intersection_time(
        sphere, &face, &bvh->faces[faces[i]].normal, expand_down);
      t = fmin(t, tmp);
    }

    assert(distance);
    *distance = t * expand_down;
    return 1;
  }
}

// NOTE: This function is different then a simple collision check, as it takes
// into account whether the capsule segment intersects the face in question or
// not.
// NOTE: to_add is the vector used to shift the capsule outside of collision 
// with the side of platforms. 
uint32_t
is_falling(
  capsule_t capsule,
  bvh_t* bvh,
  vector3f* to_add)
{
  uint32_t array[256];
  uint32_t used = 0;
  int32_t collision_count = 0;
  
  // find the aabb leaves we share space with.
  float multiplier = 1.2f;
  bvh_aabb_t bounds = { capsule.center, capsule.center };
  vector3f min, max;
  vector3f_set_3f(
    &min, 
    -capsule.radius * multiplier, 
    (-capsule.half_height - capsule.radius) * multiplier, 
    -capsule.radius * multiplier);
  vector3f_set_3f(
    &max, 
    capsule.radius * multiplier, 
    (capsule.half_height + capsule.radius) * multiplier,
    capsule.radius * multiplier);
  add_set_v3f(bounds.min_max, &min);
  add_set_v3f(bounds.min_max + 1, &max);

  query_intersection(bvh, &bounds, array, &used);
  vector3f_set_1f(to_add, 0.f);
  
  if (used) {
    bvh_aabb_t capsule_aabb;
    segment_t segment;
    vector3f penetration;
    capsule_face_classification_t classification;
    face_t face;
    segment_plane_classification_t segment_classification;
    point3f segment_intersection, segment_closest;
    point3f sphere_center;

    get_capsule_segment(&capsule, &segment);
    populate_capsule_aabb(&capsule_aabb, &capsule);

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid || !bvh->faces[i].is_floor)
          continue;

        if (!bounds_intersect(&capsule_aabb, &bvh->faces[i].aabb))
          continue;
        
        // TODO: we can avoid a copy here.
        face.points[0] = bvh->faces[i].points[0];
        face.points[1] = bvh->faces[i].points[1];
        face.points[2] = bvh->faces[i].points[2];
          
        classification = classify_capsule_face(
          &capsule,
          &face,
          &bvh->faces[i].normal,
          0,
          &penetration,
          &sphere_center);

        if (classification != CAPSULE_FACE_NO_COLLISION) {
          float t;
          segment_classification = 
            classify_segment_face(
              &face, 
              &bvh->faces[i].normal, 
              &segment, 
              &segment_intersection, 
              &t);

          if ((
            segment_classification == SEGMENT_PLANE_INTERSECT_OFF_SEGMENT ||
            segment_classification == SEGMENT_PLANE_INTERSECT_ON_SEGMENT) &&
            classify_coplanar_point_face(
              &face, 
              &bvh->faces[i].normal, 
              &segment_intersection, 
              &segment_closest) == COPLANAR_POINT_ON_OR_INSIDE) {
            
            vector3f_set_1f(to_add, 0.f);
            return 0;
          } else {
            // if it does collide with the floor outer rim, we need to find the
            // vector that pushes it outside the edge.
            add_set_v3f(to_add, &penetration);
            ++collision_count;
          }
        }
      }
    }
  }

  if (collision_count)
    mult_set_v3f(to_add, 1.f/(float)collision_count);
  
  return 1;
}

static
collision_flags_t
handle_collision_binned(
  camera_t* camera, 
  bvh_t* bvh, 
  uint32_t array[256],
  uint32_t used,
  capsule_t* capsule, 
  pipeline_t* pipeline, 
  uint32_t iterations,
  const float threshold_sqrd)
{
  collision_flags_t flags = COLLIDED_NONE;
  vector3f penetration;
  bvh_aabb_t capsule_aabb;
  capsule->center = camera->position;
  populate_capsule_aabb(&capsule_aabb, capsule);

  assert(used && "Do not call this function with used set to 0");

  while (iterations--) {
    // apply collision logic.
    capsule_face_classification_t classification;
    face_t face;
    int32_t collided = 0;
    float length_sqrd;
    vector3f displace;
    point3f sphere_center;
    vector3f_set_3f(&displace, 0.f, 0.f, 0.f);

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid)
          continue;

        if (!bounds_intersect(&capsule_aabb, &bvh->faces[i].aabb))
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

        length_sqrd = length_squared_v3f(&penetration);

        if (
          classification != CAPSULE_FACE_NO_COLLISION && 
          length_sqrd > 0.f &&
          length_sqrd < threshold_sqrd) {
          add_set_v3f(&displace, &penetration);
          ++collided;

          flags |= bvh->faces[i].is_ceiling * COLLIDED_CEILING_FLAG;
          flags |= bvh->faces[i].is_floor * COLLIDED_FLOOR_FLAG;

          if (draw_collided_face)
            draw_face(
              bvh->faces + i, 
              &bvh->faces[i].normal, 
              &blue, 
              5, 
              pipeline);
        }
      }
    }

    // don't iterate if we did not collide with anything.
    if (collided == 0)
      break;
    else {
      float multiplier = 1.0f;
      mult_set_v3f(&displace, 1.f/collided);
      mult_set_v3f(&displace, multiplier);
      add_set_v3f(&capsule->center, &displace);
      add_set_v3f(&camera->position, &displace);
    }
  }

  return flags;
}

static
float
find_quadratic_root(
  const uint32_t bins, 
  const float binned_distance)
{
  float denom = 1.f;
  for (uint32_t i = 1; i < bins; ++i)
    denom += powf(2, i);

  return binned_distance/denom;
}

static
float
get_nth_value(
  const float root,
  const uint32_t nth)
{
  float current = root;
  for (uint32_t i = 0; i < nth; ++i)
    current += root * powf(2, i + 1);
  return current;
}

// TODO(khalil): create a context that holds the information we need to pass for
// updates.
static
collision_flags_t
handle_collision(
  camera_t* camera, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline,
  const uint32_t sorted_iterations,
  const uint32_t iterations,
  const uint32_t bins,
  const float binned_distance,
  const float limit_distance)
{
  collision_flags_t flags = COLLIDED_NONE;
  vector3f penetration;
  uint32_t array[256];
  uint32_t used = 0;
  capsule->center = camera->position;

  assert(bins >= 1 && "bins have to be greater or equal to 1!");

  {
    // NOTE: Find the aabb leaves we share space with, we use a multiplier to
    // avoid doing this again in handle_collision_binned. This currently isn't
    // error proof but can be made as such by comparing the displacements to the
    // original query and recalculating if it exceeds the mutliplier bounds.
    float multiplier = 1.5f;
    bvh_aabb_t bounds = { capsule->center, capsule->center };
    vector3f min;
    vector3f_set_3f(
      &min, 
      -capsule->radius * multiplier, 
      (-capsule->half_height - capsule->radius) * multiplier, 
      -capsule->radius * multiplier);
    add_set_v3f(bounds.min_max, &min);
    mult_set_v3f(&min, -1.f);
    add_set_v3f(bounds.min_max + 1, &min);

    query_intersection(bvh, &bounds, array, &used);
    
    if (used && draw_collision_query) {
      float vertices[12];
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
      // Stick to the binned approach.
      float root = find_quadratic_root(bins, binned_distance * binned_distance);
      for (uint32_t i = 0; i < bins; ++i) {
        float threshold = get_nth_value(root, i);
        flags |= handle_collision_binned(
          camera, 
          bvh, 
          array, 
          used, 
          capsule, 
          pipeline, 
          iterations, 
          threshold);
      }

      // handle all collisions.
      flags |= handle_collision_binned(
          camera, 
          bvh, 
          array, 
          used, 
          capsule, 
          pipeline, 
          iterations, 
          limit_distance);
    }
  }

  return flags;
}

static
void
handle_physics(
  float delta_time,
  camera_t* camera, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline,
  font_runtime_t* font,
  const uint32_t font_image_id,
  const float snap_extent,
  const float snap_up_extent,
  const float snap_threshold)
{
  float multiplier = delta_time / REFERENCE_FRAME_TIME;
  uint32_t currently_falling = 0, remains_falling = 0;
  vector3f to_add;

  if (movement_mode)
    return;

  {
    // check if we are on a walking surface or not.
    capsule_t copy = *capsule;
    copy.center = camera->position;
    currently_falling = is_falling(copy, bvh, &to_add);
    copy.center.data[1] -= snap_extent;
    remains_falling = is_falling(copy, bvh, &to_add);
  }

  // snap the capsule to the nearst floor if it is no longer falling.
  if ((!currently_falling || !remains_falling) && cam_speed.data[1] <= 0.f) {
    float distance = 0.f;
    uint32_t collides = 0;
    capsule_t copy = *capsule;
    copy.center = camera->position;
    copy.center.data[1] += snap_up_extent;
    collides = snaps_to_floor_at(
      &copy, bvh, snap_extent + snap_up_extent, &distance);
    
    // TODO: This hits sometimes why????
    assert(collides && "has to collide at this point!");

    currently_falling = 0;
    cam_speed.data[1] = 0.f;
    copy.center.data[1] -= distance;
    camera->position.data[1] = copy.center.data[1];

    if (draw_status) {
      char text[512];
      const char* ptext = text;
      memset(text, 0, sizeof(text));
      sprintf(text, "SNAPPING %f", distance - snap_up_extent);
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

  if (is_key_triggered(KEY_JUMP) && !currently_falling) {
    currently_falling = 1;
    cam_speed.data[1] = jump_speed;
  }

  if (currently_falling) {
    cam_speed.data[1] -= gravity_acc * multiplier;
    cam_speed.data[1] = fmax(cam_speed.data[1], -cam_speed_limit.data[1]);
    camera->position.data[1] += cam_speed.data[1] * multiplier;

    // smooth the protrusion effect, as the snap extent might be considerable.
    if (remains_falling) {
      mult_set_v3f(&to_add, PROTRUSION_RATIO);
      add_set_v3f(&camera->position, &to_add);
    }

    if (currently_falling && remains_falling && draw_status) {
      const char* text = "FALLING";
      render_text_to_screen(
        font,
        font_image_id,
        pipeline,
        &text,
        1,
        &red,
        300.f, 300.f);
    }
  }
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
      movement_mode ? &red : &white,
      0, 180.f);
  }
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

  handle_input(delta_time, camera);

  {
    // handle collision and ceiling collision.
    collision_flags_t flags = handle_collision(
      camera, bvh, &capsule, pipeline,
      SORTED_ITERATIONS,
      ITERATIONS,
      BINS,
      BINNED_MICRO_DISTANCE,
      BINNED_MACRO_LIMIT);

    if (
      !movement_mode &&
      (flags & COLLIDED_CEILING_FLAG) == COLLIDED_CEILING_FLAG &&
      cam_speed.data[1] > 0.f)
      cam_speed.data[1] = 0.f;
  }

  handle_physics(
    delta_time, camera, bvh, &capsule, pipeline, font, font_image_id,
    SNAP_EXTENT, SNAP_UP_EXTENT, SNAP_THRESHOLD);

  draw_text(pipeline, font, font_image_id);
}