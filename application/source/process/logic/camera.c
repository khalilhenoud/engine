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
#include <collision/capsule.h>
#include <collision/segment.h>
#include <collision/face.h>
#include <collision/sphere.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <application/process/text/utils.h>

#define KEY_SPEED_PLUS      '1'
#define KEY_SPEED_MINUS     '2'
#define KEY_COLLISION_QUERY '3'
#define KEY_COLLISION_FACE  '4'
#define KEY_STRAFE_LEFT     'A'
#define KEY_STRAFE_RIGHT    'D'
#define KEY_MOVE_FWD        'W'
#define KEY_MOVE_BACK       'S'
#define KEY_MOVE_UP         'Q'
#define KEY_MOVE_DOWN       'E'
#define KEY_RESET_CAMERA    'C'
#define SNAP_THRESHOLD      1

#define ITERATIONS          16


static int32_t prev_mouse_x = -1; 
static int32_t prev_mouse_y = -1;
static int32_t draw_collision_query = 0;
static int32_t draw_collided_face = 0;
static float y_speed = 0.f;
static const float jump_speed = 20.f;
static const float y_acc = 1.f;

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

  if (is_key_triggered(KEY_COLLISION_QUERY))
    draw_collision_query = !draw_collision_query;

  if (is_key_triggered(KEY_COLLISION_FACE))
    draw_collided_face = !draw_collided_face;

  if (is_key_pressed(KEY_SPEED_PLUS))
    speed += 0.25;

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
  if (is_key_pressed(KEY_MOVE_DOWN))
    camera->position.data[1] -= speed;

  if (is_key_pressed(KEY_MOVE_UP))
    camera->position.data[1] += speed;

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
float
snap_to_floor(
  capsule_t capsule,
  bvh_t* bvh,
  float expand_down,
  pipeline_t* pipeline,
  int32_t draw_collision_query,
  int32_t draw_collision_face)
{
  float displacement = 0.f;
  float min_distance = FLT_MAX, current_distance = 0.f;
  uint32_t min_distance_index = 0;
  color_t green = { 1.f, 0.f, 0.f, 1.f };
  color_t red = { 0.f, 1.f, 0.f, 1.f };
  color_t yellow = { 1.f, 1.f, 0.f, 1.f };
  uint32_t array[256];
  uint32_t used = 0;
  
  // find the aabb leaves we share space with.
  float multiplier = 1.2f;
  bvh_aabb_t bounds;
  vector3f min, max, sphere_center;
  vector3f_set_3f(
    &sphere_center, 
    capsule.center.data[0], 
    capsule.center.data[1] - capsule.half_height, 
    capsule.center.data[2]);
  bounds.min_max[0] = bounds.min_max[1] = sphere_center;
  vector3f_set_3f(
    &min, 
    -capsule.radius * multiplier, 
    (- capsule.radius - expand_down) * multiplier, 
    -capsule.radius * multiplier);
  vector3f_set_3f(
    &max, 
    capsule.radius * multiplier, 
    capsule.radius * multiplier, 
    capsule.radius * multiplier);
  add_set_v3f(bounds.min_max, &min);
  add_set_v3f(bounds.min_max + 1, &max);

  query_intersection(bvh, &bounds, array, &used);
  
  if (used) {
    vector3f penetration;
    capsule_face_classification_t classification;
    faceplane_t face;

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid || !bvh->faces[i].is_floor)
          continue;

        if (draw_collision_query) {
          draw_face(
            bvh->faces + i,
            &bvh->faces[i].normal,
            (bvh->faces[i].is_floor) ? &green : &red,
            (bvh->faces[i].is_floor) ? 3 : 2,
            pipeline);
        }

        // TODO: we can avoid a copy here.
        face.points[0] = bvh->faces[i].points[0];
        face.points[1] = bvh->faces[i].points[1];
        face.points[2] = bvh->faces[i].points[2];

        current_distance = 
          get_point_distance(&face, &bvh->faces[i].normal, &sphere_center);

        // NOTE: We know we do not collide with any face, so we restrict
        // ourselves to distances greater than our radius.
        if (
          current_distance > capsule.radius && 
          current_distance < min_distance) {
          min_distance = current_distance;
          min_distance_index = i;
        }
      }
    }
  }

  if (used && draw_collision_face) {
     draw_face(
      bvh->faces + min_distance_index,
      &bvh->faces[min_distance_index].normal,
      &yellow,
      5,
      pipeline);
  }

  if (min_distance != FLT_MAX)
    displacement = min_distance - capsule.radius;

  return displacement;
}

int32_t
is_falling(
  capsule_t capsule,
  bvh_t* bvh,
  float expand_down,
  pipeline_t* pipeline,
  int32_t draw_collision_query,
  int32_t draw_collision_face)
{
  color_t green = { 1.f, 0.f, 0.f, 1.f };
  color_t red = { 0.f, 1.f, 0.f, 1.f };
  color_t blue = { 0.f, 0.f, 1.f, 1.f };
  uint32_t array[256];
  uint32_t used = 0;
  
  // find the aabb leaves we share space with.
  float multiplier = 1.2f;
  bvh_aabb_t bounds = { capsule.center, capsule.center };
  vector3f min, max;
  vector3f_set_3f(
    &min, 
    -capsule.radius * multiplier, 
    (-capsule.half_height - capsule.radius - expand_down) * multiplier, 
    -capsule.radius * multiplier);
  vector3f_set_3f(
    &max, 
    capsule.radius * multiplier, 
    (capsule.half_height + capsule.radius) * multiplier, 
    capsule.radius * multiplier);
  add_set_v3f(bounds.min_max, &min);
  add_set_v3f(bounds.min_max + 1, &max);

  query_intersection(bvh, &bounds, array, &used);
  
  if (used) {
    vector3f penetration;
    capsule_face_classification_t classification;
    faceplane_t face;

    for (uint32_t used_index = 0; used_index < used; ++used_index) {
      bvh_node_t* node = bvh->nodes + array[used_index];
      for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
        if (!bvh->faces[i].is_valid || !bvh->faces[i].is_floor)
          continue;

        if (draw_collision_query) {
          draw_face(
            bvh->faces + i,
            &bvh->faces[i].normal,
            (bvh->faces[i].is_floor) ? &green : &red,
            (bvh->faces[i].is_floor) ? 3 : 2,
            pipeline);
        }

        // TODO: we can avoid a copy here.
        face.points[0] = bvh->faces[i].points[0];
        face.points[1] = bvh->faces[i].points[1];
        face.points[2] = bvh->faces[i].points[2];
          
        classification = classify_capsule_faceplane(
          &capsule,
          &face,
          &bvh->faces[i].normal,
          &penetration);

        if (classification != CAPSULE_FACE_NO_COLLISION) {
          if (draw_collision_face)
            draw_face(
              bvh->faces + i,
              &bvh->faces[i].normal,
              &blue,
              5,
              pipeline);
          return 0;
        }
      }
    }
  }

  return 1;
}

// TODO(khalil): create a context that holds the information we need to pass for
// updates.
void
handle_collision(
  camera_t* camera, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline)
{
  color_t green = { 1.f, 0.f, 0.f, 1.f };
  color_t red = { 0.f, 1.f, 0.f, 1.f };
  color_t blue = { 0.f, 0.f, 1.f, 1.f };
  vector3f penetration;
  uint32_t array[256];
  uint32_t used = 0;
  uint32_t iterations = ITERATIONS;
  capsule->center = camera->position;

  while (iterations--) {
    // find the aabb leaves we share space with.
    float multiplier = 1.2f;
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
            (bvh->faces[i].is_floor) ? &green : &red, 
            (bvh->faces[i].is_floor) ? 3 : 2,
            pipeline);
        }
      }
    }

    // apply collision logic.
    if (used) {
      capsule_face_classification_t classification;
      faceplane_t face;
      int32_t collided = 0;
      vector3f displace;
      vector3f_set_3f(&displace, 0.f, 0.f, 0.f);

      for (uint32_t used_index = 0; used_index < used; ++used_index) {
        bvh_node_t* node = bvh->nodes + array[used_index];
        for (uint32_t i = node->first_prim; i < node->last_prim; ++i) {
          if (!bvh->faces[i].is_valid)
            continue;

          // TODO(khalil): we can avoid the copy by providing an alternative to
          // the function or a cast the 3 float vecs to faceplane_t.
          face.points[0] = bvh->faces[i].points[0]; 
          face.points[1] = bvh->faces[i].points[1]; 
          face.points[2] = bvh->faces[i].points[2]; 
            
          classification =
          classify_capsule_faceplane(
            capsule,
            &face,
            &bvh->faces[i].normal,
            &penetration);
            
          if (classification != CAPSULE_FACE_NO_COLLISION) {
            add_set_v3f(&displace, &penetration);
            ++collided;
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
        mult_set_v3f(&displace, 1.f/collided);
        add_set_v3f(&capsule->center, &displace);
        add_set_v3f(&camera->position, &displace);
      }
    }
  }
}

static
void
handle_physics(
  camera_t* camera, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline,
  font_runtime_t* font,
  uint32_t font_image_id)
{
  int32_t falling = 0;
  int32_t future_falling = 0;

  capsule_t copy = *capsule;
  copy.center = camera->position;
  falling = is_falling(copy, bvh, 0.f, pipeline, 0, 0);
  copy.center.data[1] -= capsule->radius;
  future_falling = is_falling(copy, bvh, 0.f, pipeline, 0, 0);

  // snap the capsule to the nearst floor.
  if (falling && !future_falling && y_speed <= 0.f) {
    float snap_distance = 0.f;
    capsule_t new_copy = *capsule;
    new_copy.center = camera->position;
    snap_distance = snap_to_floor(new_copy, bvh, capsule->radius, pipeline, 0, 0);
    if (snap_distance > SNAP_THRESHOLD)
      camera->position.data[1] -= snap_distance;
    falling = 0;
    y_speed = 0.f;
  }

  if (is_key_triggered(0x20) && !falling) {
    falling = 1;
    y_speed = jump_speed;
  }

  if (falling) {
    y_speed -= y_acc;
    y_speed = fmax(y_speed, -jump_speed);
    camera->position.data[1] += y_speed;

    if (falling && future_falling) {
      color_t red = { 1.f, 0.f, 0.f, 1.f };
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

void
camera_update(
  camera_t* camera, 
  bvh_t* bvh, 
  capsule_t* capsule, 
  pipeline_t* pipeline,
  font_runtime_t* font, 
  uint32_t font_image_id)
{
  handle_input(camera);
  handle_physics(camera, bvh, capsule, pipeline, font, font_image_id);
  handle_collision(camera, bvh, capsule, pipeline);
}