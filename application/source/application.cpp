/**
 * @file application.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <windows.h>
#include <cassert>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <entity/c/runtime/texture.h>
#include <entity/c/runtime/texture_utils.h>
#include <entity/c/mesh/color.h>
#include <entity/c/mesh/mesh.h>
#include <entity/c/mesh/mesh_utils.h>
#include <entity/c/scene/scene_utils.h>
#include <entity/c/scene/scene.h>
#include <entity/c/runtime/font.h>
#include <entity/c/runtime/font_utils.h>
#include <entity/c/scene/camera.h>
#include <entity/c/scene/camera_utils.h>
#include <application/application.h>
#include <application/framerate_controller.h>
#include <application/input.h>
#include <application/converters/to_render_data.h>
#include <application/converters/png_to_image.h>
#include <application/converters/csv_to_font.h>
#include <application/converters/bin_to_scene_to_bin.h>
#include <application/process/text/utils.h>
#include <math/vector3f.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <collision/capsule.h>
#include <collision/segment.h>
#include <collision/face.h>
#include <collision/sphere.h>
#include <serializer/serializer_bin.h>


////////////////////////////////////////////////////////////////////////////////
std::vector<uintptr_t> allocated;

void* allocate(size_t size)
{
  void* block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void free_block(void* block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(), 
      allocated.end(), 
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }), 
    allocated.end());
  free(block);
}

////////////////////////////////////////////////////////////////////////////////

// TODO: Move all of this stuff into a collision app/unit test. It is useful but
// not here.
#define SHOW_GRID 0
#define RENDER_SCENE !SHOW_GRID
#define COLLISION_LOGIC 0
#define COLLISION_LOGIC_PHYSICS 0
#define NEW_COLLISION_STUFF 0
#define DRAW_TEXT 1

framerate_controller controller;
pipeline_t pipeline;

// the scene and its renderer data.
scene_t* scene;
render_data_t* scene_render_data;

// the capsule and its renderer data.
capsule_t capsule;
mesh_t* capsule_mesh;
render_data_t* capsule_render_data;

// The font in question.
font_runtime_t* font;
image_t* font_image;
uint32_t font_image_id;

camera_t* camera;

#if 0
// std::unordered_map<std::string, uint32_t> texture_id;
entity::camera m_camera;
std::vector<entity::image> m_images;

std::vector<const char*> texture_render_data;
std::vector<uint32_t> texture_render_id;
#endif
allocator_t allocator;


#if NEW_COLLISION_STUFF
mesh_t* meshes[2] = { nullptr };
std::vector<mesh_render_data_t> collision_render_data;
capsule_t capsules[2];
face_t faces[2];
int32_t chosen_index = 0;
int32_t total_index = 6;
int32_t face_indices[6] = { 583, 588 , 576 , 589 , 576 , 589 };
vector3f positions[6] = { 
  {1802.03906, -107.383759, 34.1438942},
  {2312.48413, -205.961578, -876.927917}, 
  {2293.82910, -218.396912, -860.903931}, 
  {2298.09180, -163.541550, -868.215210}, 
  {2287.18262, -213.384109, -869.126343}, 
  {2291.02075, -163.990952, -875.709595} };
vector3f points[3] = {
  {1838.62024, -135.575882, -4.60462284},
  {1838.62024, -135.575882, -4.60462284},
  {1839.68018, -135.092865, -4.60462952}
};
vector3f colors[6] = {
  {1.f, 0.f, 1.f},
  {1.f, 0.f, 1.f},
  {0.f, 1.f, 1.f},
  {1.f, 1.f, 1.f},
  {0.f, 1.f, 0.f},
  {1.f, 0.f, 1.f} };
float m_offset[3] = {0};
#endif

#if COLLISION_LOGIC
#if COLLISION_LOGIC_PHYSICS
const float y_acc = 1.f;
float y_speed = 0.f;  
#endif
bool draw_debug = false;
capsule_t capsule;
mesh_t* capsule_mesh;
mesh_render_data_t capsule_render_data;
#endif
#if 0
std::vector<face_t> collision_faces;
std::vector<vector3f> collision_normals;
std::vector<float> dot_product_normals;
std::vector<bool> is_floor;
std::vector<bool> is_face_valid;
#endif

application::application(
  int32_t width,
  int32_t height,
  const char* dataset)
  : m_dataset(dataset)
{
  ::renderer_initialize();

  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = nullptr;
  allocator.mem_realloc = nullptr;

  camera = create_camera(&allocator);
  {
    vector3f center;
    center.data[0] = center.data[1] = center.data[2] = 0.f;
    vector3f at;
    at.data[0] = at.data[1] = at.data[2] = 0.f;
    at.data[2] = -1.f;
    vector3f up;
    up.data[0] = up.data[1] = up.data[2] = 0.f;
    up.data[1] = 1.f;
    ::set_camera_lookat(camera, center, at, up);
  }

#if NEW_COLLISION_STUFF
  capsules[0].center = { 2292.47876, -162.489792, -863.596619 };
  capsules[0].half_height = 30;
  capsules[0].radius = 25;
  meshes[0] = ::create_unit_capsule(30, capsules[0].half_height / capsules[0].radius, &allocator);
  collision_render_data.push_back(load_mesh_renderer_data(meshes[0], color_t{ 1.f, 1.f, 0.f, 0.5f }));

  capsules[1].center = { 2290.51538, -216.135056, -860.228882 };
  capsules[1].half_height = 30;
  capsules[1].radius = 25;
  meshes[1] = ::create_unit_capsule(30, capsules[1].half_height / capsules[1].radius, &allocator);
  collision_render_data.push_back(load_mesh_renderer_data(meshes[1], color_t{ 1.f, 0.f, 1.f, 0.5f }));

  faces[0].points[0] = { 2401.75513, -216.838501, -792.779297 };
  faces[0].points[1] = { 2228.06006, -168.812302, -984.672241 };
  faces[0].points[2] = { 2203.34351, -216.838501, -908.459717 };

  faces[1].points[0] = { 2051.03491, -95.3237305, -85.5542297 };
  faces[1].points[1] = { 2401.75513, -216.838501, -792.779297 };
  faces[1].points[2] = { 2203.34351, -216.838501, -908.459717 };

  m_camera.m_position.x = 2300;
  m_camera.m_position.y = -200;
  m_camera.m_position.z = -600;
#endif

#if 0
  auto datafile = std::string("media\\font\\FontData2.csv");
  auto imagefile = std::string("media\\font\\ExportedFont2.png");
  m_font = load_font(m_dataset, imagefile, datafile, &allocator);
  m_images.emplace_back(imagefile);

  auto model_path = std::string("media\\test\\test01.ASE");
  //m_scene = load_ase_model(m_dataset, model_path, &allocator);

  //{
  //  // save to bin format.
  //  auto fullpath = m_dataset + "media\\cooked\\test01.bin";
  //  auto scene_bin = scene_to_bin(*m_scene, &allocator);
  //  ::serialize_bin(fullpath.c_str(), scene_bin);
  //  ::free_bin(scene_bin, &allocator);
  //}
#endif
  {
    // load the scene from a bin file.
    auto fullpath = m_dataset + "media\\cooked\\map.bin";
    serializer_scene_data_t *scene_bin = ::deserialize_bin(
      fullpath.c_str(), &allocator);
    scene_bin->material_repo.data[0].ambient.data[0] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[1] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[2] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[3] = 1.f;
    scene = bin_to_scene(scene_bin, &allocator);
    ::free_bin(scene_bin, &allocator);
  }

  // load the scene render data.
  scene_render_data = load_scene_render_data(scene, &allocator);

  // need to load the images required by the scene.

  // load the capsule mesh.
  {
    memset(capsule.center.data, 0, sizeof(capsule.center.data));
    capsule.half_height = 30;
    capsule.radius = 25;
    capsule_mesh = create_unit_capsule(
      30, capsule.half_height / capsule.radius, &allocator);
    capsule_render_data = load_mesh_renderer_data(
      capsule_mesh, color_rgba_t{ 1.f, 1.f, 0.f, 0.5f }, &allocator);
  }

  // load the font in question.
  {
    auto datafile = std::string("media\\font\\FontData.csv");
    auto imagefile = std::string("media\\font\\ExportedFont.png");
    font = load_font(
      m_dataset.c_str(), imagefile.c_str(), datafile.c_str(), &allocator);
    
    {
      texture_t texture;
      memset(texture.path.data, 0, sizeof(texture.path.data));
      memcpy(texture.path.data, imagefile.c_str(), imagefile.size());
      font_image = create_texture_runtime(&texture, &allocator);
      load_image_buffer(m_dataset.c_str(), font_image, &allocator);
      font_image_id = ::upload_to_gpu(
        font_image->texture.path.data,
        font_image->buffer,
        font_image->width,
        font_image->height,
        (renderer_image_format_t)font_image->format);
    }
  }

#if 0
  std::tie(mesh_render_data, texture_render_data) = load_renderer_data(*m_scene);

  for (auto& entry : m_scene->get_textures_paths())
    m_images.emplace_back(entry.c_str());

  for (auto& image : m_images)
    load_image(m_dataset, image, &allocator);

  for (auto& image : m_images) {
    if (texture_id.find(image.m_path) != texture_id.end())
      continue;
    
    texture_id[image.m_path] = ::upload_to_gpu(
      image.m_path.c_str(),
      &image.m_buffer[0],
      image.m_width,
      image.m_height,
      static_cast<image_format_t>(image.m_format));
  }

  for (auto path : texture_render_data) {
    if (path && (texture_id.find(std::string(path)) != texture_id.end())) 
      texture_render_id.push_back(texture_id[std::string(path)]);
    else
      texture_render_id.push_back(0);
  }

#if COLLISION_LOGIC
  {
    capsule.center = m_camera.m_position.data;
    capsule.half_height = 30;
    capsule.radius = 25;
    capsule_mesh = ::create_unit_capsule(30, capsule.half_height / capsule.radius, &allocator);
    capsule_render_data = load_mesh_renderer_data(capsule_mesh, color_t{ 1.f, 1.f, 0.f, 0.5f });
  }
#endif
  {
    // use the scene mesh_faces.
    auto meshes = m_scene->get_meshes();
    for (auto * mesh : meshes) {
      for (uint32_t i = 0; i < mesh->m_indices.size(); i+=3) {
        face_t face;
        uint32_t indices[3] = { 
          mesh->m_indices[i + 0], 
          mesh->m_indices[i + 1], 
          mesh->m_indices[i + 2] };
        
        for (uint32_t k = 0; k < 3; ++k) {
          face.points[k].data[0] = mesh->m_vertices[indices[k] * 3 + 0];
          face.points[k].data[1] = mesh->m_vertices[indices[k] * 3 + 1];
          face.points[k].data[2] = mesh->m_vertices[indices[k] * 3 + 2];
        }
        
        collision_faces.push_back(face);
      }
    }

    collision_normals.resize(collision_faces.size());
    get_faces_normals(
      &collision_faces[0], 
      collision_faces.size(), 
      &collision_normals[0]);

    float cosine_target = cosf(TO_RADIANS(80));
    for (uint32_t i = 0, count = collision_faces.size(); i < count; ++i) {
      // only require the up component
      float value = collision_normals[i].data[1];
      dot_product_normals.push_back(value);
      is_floor.push_back(value > cosine_target);

      if (
          ::equal_to_v3f(collision_faces[i].points + 0, collision_faces[i].points + 1) ||
          ::equal_to_v3f(collision_faces[i].points + 0, collision_faces[i].points + 2) ||
          ::equal_to_v3f(collision_faces[i].points + 1, collision_faces[i].points + 2))
          is_face_valid.push_back(false);
        else
          is_face_valid.push_back(true);
    }
  }
#endif

  ::pipeline_set_default(&pipeline);
  ::set_viewport(&pipeline, 0.f, 0.f, float(width), float(height));
  ::update_viewport(&pipeline);

  /// "http://stackoverflow.com/questions/12943164/replacement-for-gluperspective-with-glfrustrum"
  float znear = 0.1f, zfar = 4000.f, aspect = (float)width / height;
  float fh = (float)tan((double)60.f / 2.f / 180.f * K_PI) * znear;
  float fw = fh * aspect;
  ::set_perspective(&pipeline, -fw, fw, -fh, fh, znear, zfar);
  ::update_projection(&pipeline);

  controller.lock_framerate(60);

  ::show_cursor(0);
}

application::~application()
{
#if NEW_COLLISION_STUFF
  ::free_mesh(meshes[0], &allocator);
  ::free_mesh(meshes[1], &allocator);
#endif

#if COLLISION_LOGIC
  {
    ::free_mesh(capsule_mesh, &allocator);
  }
#endif

#if 0
  for (auto& entry : texture_id)
    ::evict_from_gpu(entry.second);
#endif
  ::free_camera(camera, &allocator);
  ::free_scene(scene, &allocator);
  ::free_render_data(scene_render_data, &allocator);
  ::free_mesh(capsule_mesh, &allocator);
  ::free_render_data(capsule_render_data, &allocator);
  ::free_font_runtime(font, &allocator);
  ::free_texture_runtime(font_image, &allocator);
  ::evict_from_gpu(font_image_id);
  ::renderer_cleanup();

  assert(allocated.size() == 0 && "Memory leak detected!");
}

uint64_t 
application::update()
{
  controller.start();

  ::input_update();
  ::clear_color_and_depth_buffers();

  // disable/enable input with '~' key.
  if (::is_key_triggered(VK_OEM_3)) {
    m_disable_input = !m_disable_input;
    ::show_cursor((int32_t)m_disable_input);

    if (m_disable_input)
      prev_mouse_x = prev_mouse_y = -1;
  }

  static int32_t input_static = 0;
  if (!m_disable_input) {
    update_camera();    

    if (::is_key_triggered('T')) {
      ++input_static;
      input_static %= 3;
    }

#if COLLISION_LOGIC
    if (::is_key_triggered('G'))
      draw_debug = !draw_debug;
#endif
  }

  matrix4f out;
  memset(&out, 0, sizeof(matrix4f));
  ::get_view_transformation(camera, &out);
  ::set_matrix_mode(&pipeline, MODELVIEW);
  ::load_identity(&pipeline);
  ::post_multiply(&pipeline, &out);

#if SHOW_GRID
  // draw grid.
  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, -100, 0);
    ::draw_grid(&pipeline, 5000.f, 100);
    ::pop_matrix(&pipeline);
  }
#endif

#if RENDER_SCENE
  if (scene) {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, 0, 0);
    ::pre_scale(&pipeline, 1, 1, 1);
    ::draw_meshes(
    scene_render_data->mesh_render_data, 
    scene_render_data->texture_ids, 
    scene_render_data->mesh_count, 
    &pipeline);
   ::pop_matrix(&pipeline);
  }
#endif

#if COLLISION_LOGIC
  {
    uint32_t capsule_texture_render_id[1] = { 0 };
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, capsule.center.data[0], capsule.center.data[1], capsule.center.data[2]);
    float scale = (capsule.half_height + capsule.radius);
    ::pre_scale(&pipeline, scale, scale, scale);
    ::draw_meshes(&capsule_render_data, capsule_texture_render_id, 1, &pipeline);
    ::pop_matrix(&pipeline);
  }
#endif

#if NEW_COLLISION_STUFF
  {
    if (::is_key_triggered('T')) {
      chosen_index++;
      chosen_index %= total_index;
    }

    if (::is_key_pressed('H'))
      m_offset[0] -= 2.f;
    if (::is_key_pressed('K'))
      m_offset[0] += 2.f;
    if (::is_key_pressed('U'))
      m_offset[1] += 2.f;
    if (::is_key_pressed('J'))
      m_offset[1] -= 2.f;
    if (::is_key_pressed('Y'))
      m_offset[2] += 2.f;
    if (::is_key_pressed('I'))
      m_offset[2] -= 2.f;

    {
      {
        {
          segment_t segments[2];
          segments[0].points[0] = { 1926.95251, -95.3237305, -157.898224 };
          segments[0].points[1] = { 1806.96692, -150.000000, 52.8562088 };
          segments[1].points[0] = { 1854.29382, -128.433578, -4.60462284 };
          segments[1].points[1] = { 1838.62024, -135.575882, -4.60462284 };

          for (int32_t i = 0; i < 2; ++i) {
            float vertices[12];
            vertices[0 * 3 + 0] = segments[i].points[0].data[0];
            vertices[0 * 3 + 1] = segments[i].points[0].data[1];
            vertices[0 * 3 + 2] = segments[i].points[0].data[2];
            vertices[1 * 3 + 0] = segments[i].points[1].data[0];
            vertices[1 * 3 + 1] = segments[i].points[1].data[1];
            vertices[1 * 3 + 2] = segments[i].points[1].data[2];
            ::draw_lines(vertices, 2, { 0.f, 1.f, 1.f, 1.f }, 1, & pipeline);
          }
        }

        {
          ::draw_points(points[0].data, 3, {1.f, 0.f, 0.f, 1.f}, 3, & pipeline);
        }
      }

      {
        for (/*int32_t i = 0; i < 5; ++i*/;;) {
          int32_t i = chosen_index;
          face_t& face = collision_faces[face_indices[i]];
          float vertices[12];
          vertices[0 * 3 + 0] = face.points[0].data[0];
          vertices[0 * 3 + 1] = face.points[0].data[1];
          vertices[0 * 3 + 2] = face.points[0].data[2];
          vertices[1 * 3 + 0] = face.points[1].data[0];
          vertices[1 * 3 + 1] = face.points[1].data[1];
          vertices[1 * 3 + 2] = face.points[1].data[2];
          vertices[2 * 3 + 0] = face.points[2].data[0];
          vertices[2 * 3 + 1] = face.points[2].data[1];
          vertices[2 * 3 + 2] = face.points[2].data[2];
          vertices[3 * 3 + 0] = face.points[0].data[0];
          vertices[3 * 3 + 1] = face.points[0].data[1];
          vertices[3 * 3 + 2] = face.points[0].data[2];
          ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);
          break;
        }
      }

      {
        for (/*int32_t i = 0; i < 5; ++i*/;;) {
          int32_t i = chosen_index;
          vector3f offset = { m_offset[0], m_offset[1], m_offset[2] };
          capsules[0].center = add_v3f(positions + i, &offset);
          face_t& face = collision_faces[face_indices[i]];
          vector3f& normal = collision_normals[face_indices[i]];
          bool floor = is_floor[face_indices[i]];

          {
            segment_t segment;
            get_capsule_segment(capsules, &segment);

            float vertices[12];
            vertices[0 * 3 + 0] = segment.points[0].data[0];
            vertices[0 * 3 + 1] = segment.points[0].data[1];
            vertices[0 * 3 + 2] = segment.points[0].data[2];
            vertices[1 * 3 + 0] = segment.points[1].data[0];
            vertices[1 * 3 + 1] = segment.points[1].data[1];
            vertices[1 * 3 + 2] = segment.points[1].data[2];
            ::draw_lines(vertices, 2, { 0.f, 1.f, 1.f, 1.f }, 1, &pipeline);
          }
         

          ::push_matrix(&pipeline);
          ::pre_translate(&pipeline, capsules[0].center.data[0], capsules[0].center.data[1], capsules[0].center.data[2]);

          {
            vector3f result;
            segment_t coplanar_overlap;
            capsule_face_classification_t classification =
              classify_capsule_face(&capsules[0], &face, &normal, &result, &coplanar_overlap);
            if (classification != CAPSULE_FACE_NO_COLLISION) {
              float direction[6];
              float length0 = ::length_v3f(&result);
              float length1 = -(capsules[0].radius - length0);
              result = normalize_v3f(&result);
              direction[0] = result.data[0] * length1;
              direction[1] = result.data[1] * length1;
              direction[2] = result.data[2] * length1;
              direction[3] = direction[0] + result.data[0] * length0;
              direction[4] = direction[1] + result.data[1] * length0;
              direction[5] = direction[2] + result.data[2] * length0;
              if (classification == CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_INTERSECTS_FACE)
                ::draw_lines(direction, 2, color_t{ 1.f, 0.f, 0.f, 1.f }, 2, &pipeline);
              else
                ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);

              if (classification == CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE) {
                ::pop_matrix(&pipeline);
                ::draw_lines(coplanar_overlap.points[0].data, 2, color_t{ 0, 1, 0, 1 }, 5, &pipeline);
                ::push_matrix(&pipeline);
                ::pre_translate(&pipeline, capsules[0].center.data[0], capsules[0].center.data[1], capsules[0].center.data[2]);
              }
            }
          }

          ::pop_matrix(&pipeline);

          {
            uint32_t capsule_texture_render_id[1] = { 0 };
            ::push_matrix(&pipeline);
            ::pre_translate(&pipeline, capsules[0].center.data[0], capsules[0].center.data[1], capsules[0].center.data[2]);
            float scale = (capsules[0].half_height + capsules[0].radius);
            ::pre_scale(&pipeline, scale, scale, scale);
            ::draw_meshes(&collision_render_data[0], capsule_texture_render_id, 1, &pipeline);
            ::pop_matrix(&pipeline);
          }

          break;
        }
      }
    }
  }
#endif

#if COLLISION_LOGIC
  {
    if (draw_debug) {
      // draw the collision faces.
      float vertices[12];
      for (uint32_t i = 0; i < collision_faces.size(); ++i) {
        vertices[0 * 3 + 0] = collision_faces[i].points[0].data[0] + collision_normals[i].data[0];
        vertices[0 * 3 + 1] = collision_faces[i].points[0].data[1] + collision_normals[i].data[1];
        vertices[0 * 3 + 2] = collision_faces[i].points[0].data[2] + collision_normals[i].data[2];
        vertices[1 * 3 + 0] = collision_faces[i].points[1].data[0] + collision_normals[i].data[0];
        vertices[1 * 3 + 1] = collision_faces[i].points[1].data[1] + collision_normals[i].data[1];
        vertices[1 * 3 + 2] = collision_faces[i].points[1].data[2] + collision_normals[i].data[2];
        vertices[2 * 3 + 0] = collision_faces[i].points[2].data[0] + collision_normals[i].data[0];
        vertices[2 * 3 + 1] = collision_faces[i].points[2].data[1] + collision_normals[i].data[1];
        vertices[2 * 3 + 2] = collision_faces[i].points[2].data[2] + collision_normals[i].data[2];
        vertices[3 * 3 + 0] = collision_faces[i].points[0].data[0] + collision_normals[i].data[0];
        vertices[3 * 3 + 1] = collision_faces[i].points[0].data[1] + collision_normals[i].data[1];
        vertices[3 * 3 + 2] = collision_faces[i].points[0].data[2] + collision_normals[i].data[2];
        if (is_floor[i])
          ::draw_lines(vertices, 4, { 1.f, 0.f, 0.f, 1.f }, 3, &pipeline);
        else
          ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);
      }
    }
  }
#endif


  {
    // display simple instructions.
    std::vector<const char*> text;
    text.push_back("[C] RESET CAMERA");
    text.push_back("[~] CAMERA UNLOCK/LOCK");
    render_text_to_screen(
      font, 
      font_image_id, 
      &pipeline, 
      &text[0], 
      (uint32_t)text.size());
  }

  ::flush_operations();

  return controller.end();
}

void 
application::update_camera()
{
  matrix4f crossup, camerarotateY, camerarotatetemp, tmpmatrix;
  float dx, dy, d, tempangle;
  static float speed = 10.f;
  static bool falling = false;
  float length;
  int32_t mx, my;
  vector3f tmp, tmp2;

  if (::is_key_pressed('1')) {
    speed += 0.25;
  }

  if (::is_key_pressed('2')) {
    speed -= 0.25;
    speed = fmax(speed, 0.125);
  }

  if (::is_key_pressed('C')) {
    y_limit = 0;
    vector3f center;
    center.data[0] = center.data[1] = center.data[2] = 0.f;
    vector3f at;
    at.data[0] = at.data[1] = at.data[2] = 0.f;
    at.data[2] = -1.f;
    vector3f up;
    up.data[0] = up.data[1] = up.data[2] = 0.f;
    up.data[1] = 1.f;
    ::set_camera_lookat(camera, center, at, up);
  }

  if (prev_mouse_x == -1)
    ::center_cursor();

  // Handling the m_camera controls, first orientation, second translation.
  ::get_position(&mx, &my);
  mouse_x = mx;
  mouse_y = my;
  if (prev_mouse_x == -1)
    prev_mouse_x = mx;
  if (prev_mouse_y == -1)
    prev_mouse_y = my;
  dx = (float)mouse_x - prev_mouse_x;
  dy = (float)mouse_y - prev_mouse_y;
  ::set_position(prev_mouse_x, prev_mouse_y);

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
  camera->lookat_direction = mult_m4f_v3f(&tmpmatrix, &camera->lookat_direction);
  camera->up_vector = mult_m4f_v3f(&tmpmatrix, &camera->up_vector);

  // Handling translations.
  if (::is_key_pressed('E')) {
    camera->position.data[1] -= speed;
  }

  if (::is_key_pressed('Q')) {
    camera->position.data[1] += speed;
  }

  if (::is_key_pressed('A')) {
    camera->position.data[0] -= tmp.data[0] * speed;
    camera->position.data[2] -= tmp.data[2] * speed;
  }

  if (::is_key_pressed('D')) {
    camera->position.data[0] += tmp.data[0] * speed;
    camera->position.data[2] += tmp.data[2] * speed;
  }

  static bool k_engaged = false;

  if (::is_key_triggered('N'))
    k_engaged = !k_engaged;

  if (::is_key_pressed('W') || k_engaged) {
    math::vector3f vecxz;
    vecxz.x = camera->lookat_direction.data[0];
    vecxz.z = camera->lookat_direction.data[2];
    length = sqrtf(vecxz.x * vecxz.x + vecxz.z * vecxz.z);
    if (length != 0) {
      vecxz.x /= length;
      vecxz.z /= length;
      camera->position.data[0] += vecxz.x * speed;
      camera->position.data[2] += vecxz.z * speed;
    }
  }

  if (::is_key_pressed('S')) {
    math::vector3f vecxz;
    vecxz.x = camera->lookat_direction.data[0];
    vecxz.z = camera->lookat_direction.data[2];
    length = sqrtf(vecxz.x * vecxz.x + vecxz.z * vecxz.z);
    if (length != 0) {
      vecxz.x /= length;
      vecxz.z /= length;
      camera->position.data[0] -= vecxz.x * speed;
      camera->position.data[2] -= vecxz.z * speed;
    }
  }

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
      ::draw_lines(vertices, 4, { 1.f, 0.f, 0.f, 1.f }, 3, &pipeline);
    };

    auto&& is_falling = [&](capsule_t to_test, math::vector3f pos)-> bool {
      to_test.center = pos.data;
      for (uint32_t i = 0; i < collision_faces.size(); ++i) {
        if (!is_face_valid[i])
          continue;

        // won't consider any face who's normal isn't within a certain range.
        // this can be cached for our purposes
        if (!is_floor[i])
          continue;

        ::vector3f any;
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
    if (::is_key_triggered(0x20) /*&& !falling*/) {
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
        ::add_set_v3f(&capsule.center, &penetration);
        ::add_set_v3f(&m_camera.m_position.data, &penetration);
        draw_face(i);
      }
    }
  }
#endif
}