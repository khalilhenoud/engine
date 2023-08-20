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
#include <entity/camera.h>
#include <entity/node.h>
#include <entity/font.h>
#include <entity/image.h>
#include <application/application.h>
#include <application/framerate_controller.h>
#include <application/input.h>
#include <application/converters/ase_to_entity.h>
#include <application/converters/entity_to_render_data.h>
#include <application/converters/mesh_to_render_data.h>
#include <application/converters/png_to_image.h>
#include <application/converters/csv_to_font.h>
#include <application/converters/entity_to_bin.h>
#include <application/converters/bin_to_entity.h>
#include <math/vector3f.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <data_format/mesh_format/data_format.h>
#include <data_format/mesh_format/utils.h>
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
#define OLD_COLLISION_STUFF 0
#define COLLISION_LOGIC 1
#define COLLISION_LOGIC_PHYSICS 1
#define NEW_COLLISION_STUFF 0

framerate_controller controller;
pipeline_t pipeline;
entity::camera m_camera;
std::vector<entity::image> m_images;
std::unique_ptr<entity::node> m_scene;
std::unique_ptr<entity::font> m_font;
std::unordered_map<std::string, uint32_t> texture_id;
std::vector<mesh_render_data_t> mesh_render_data;
std::vector<const char*> texture_render_data;
std::vector<uint32_t> texture_render_id;
allocator_t allocator;

#if OLD_COLLISION_STUFF
mesh_t* meshes[4] = { nullptr };
std::vector<mesh_render_data_t> collision_render_data;
sphere_t sphere_0;
sphere_t sphere_1;
capsule_t capsule_0;
capsule_t capsule_1;
face_t face_0;
face_t face_1;
#endif

#if NEW_COLLISION_STUFF
mesh_t* meshes[2] = { nullptr };
std::vector<mesh_render_data_t> collision_render_data;
capsule_t capsules[2];
face_t faces[2];
int32_t chosen_index = 0;
int32_t total_index = 6;
int32_t face_indices[6] = { 561, 588 , 576 , 589 , 576 , 589 };
vector3f positions[6] = { 
  {1854.29382, -139.971054, -4.60462284},
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
std::vector<face_t> collision_faces;
std::vector<vector3f> collision_normals;


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

#if OLD_COLLISION_STUFF
  {
    sphere_0.center = { -100.f, 0.f, -200.f };
    sphere_0.radius = 20;
    meshes[0] = ::create_unit_sphere(30, &allocator);
    collision_render_data.push_back(load_mesh_renderer_data(meshes[0], color_t{ 0.f, 1.f, 0.f, 0.5f }));
    sphere_1.center = { 0.f, 0.f, -200.f };
    sphere_1.radius = 30;
    meshes[1] = ::create_unit_sphere(30, &allocator);
    collision_render_data.push_back(load_mesh_renderer_data(meshes[1], color_t{ 1.f, 0.f, 0.f, 0.5f }));
    capsule_0.center = { 100.f, 0.f, -200.f };
    capsule_0.half_height = 30;
    capsule_0.radius = 15;
    meshes[2] = ::create_unit_capsule(30, capsule_0.half_height / capsule_0.radius, &allocator);
    collision_render_data.push_back(load_mesh_renderer_data(meshes[2], color_t{ 1.f, 1.f, 0.f, 0.5f }));
    capsule_1.center = { 170.f, 0.f, -200.f };
    capsule_1.half_height = 20;
    capsule_1.radius = 10;
    meshes[3] = ::create_unit_capsule(30, capsule_1.half_height / capsule_1.radius, &allocator);
    collision_render_data.push_back(load_mesh_renderer_data(meshes[3], color_t{ 1.f, 0.f, 1.f, 0.5f }));

    face_0.points[0] = { 0.f, -100.f, -200.f};
    face_0.points[1] = { 50.f, -100.f, -200.f};
    face_0.points[2] = { 50.f, -100.f, -250.f};

    face_1.points[0] = { 200.f, -100.f, -200.f};
    face_1.points[1] = { 200.f, -50.f, -200.f};
    face_1.points[2] = { 200.f, -50.f, -250.f};
  }
#endif

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
  {
    // load from bin
    auto fullpath = m_dataset + "media\\cooked\\map.bin";
    serializer_scene_data_t *scene_bin = ::deserialize_bin(
      fullpath.c_str(), &allocator);
    scene_bin->material_repo.data[0].ambient.data[0] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[1] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[2] = 0.2f;
    scene_bin->material_repo.data[0].ambient.data[3] = 1.f;
    m_scene = bin_to_scene(scene_bin);
    ::free_bin(scene_bin, &allocator);
  }
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
  }

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
#if OLD_COLLISION_STUFF
  ::free_mesh(meshes[0], &allocator);
  ::free_mesh(meshes[1], &allocator);
  ::free_mesh(meshes[2], &allocator);
  ::free_mesh(meshes[3], &allocator);
#endif

#if NEW_COLLISION_STUFF
  ::free_mesh(meshes[0], &allocator);
  ::free_mesh(meshes[1], &allocator);
#endif

#if COLLISION_LOGIC
  {
    ::free_mesh(capsule_mesh, &allocator);
  }
#endif

  for (auto& entry : texture_id)
    ::evict_from_gpu(entry.second);
  
  ::renderer_cleanup(); 

  assert(allocated.size() == 0);
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

#if OLD_COLLISION_STUFF
    if (input_static == 0) {
      if (::is_key_pressed('H'))
        capsule_0.center.data[0] -= 2.f;
      if (::is_key_pressed('K'))
        capsule_0.center.data[0] += 2.f;
      if (::is_key_pressed('U'))
        capsule_0.center.data[1] += 2.f;
      if (::is_key_pressed('J'))
        capsule_0.center.data[1] -= 2.f;
      if (::is_key_pressed('Y'))
        capsule_0.center.data[2] += 2.f;
      if (::is_key_pressed('I'))
        capsule_0.center.data[2] -= 2.f;
    }
    else if (input_static == 1) {
      if (::is_key_pressed('H'))
        sphere_0.center.data[0] -= 2.f;
      if (::is_key_pressed('K'))
        sphere_0.center.data[0] += 2.f;
      if (::is_key_pressed('U'))
        sphere_0.center.data[1] += 2.f;
      if (::is_key_pressed('J'))
        sphere_0.center.data[1] -= 2.f;
      if (::is_key_pressed('Y'))
        sphere_0.center.data[2] += 2.f;
      if (::is_key_pressed('I'))
        sphere_0.center.data[2] -= 2.f;
    }
#endif
  }


  ::set_matrix_mode(&pipeline, MODELVIEW);
  ::load_identity(&pipeline);
  ::post_multiply(&pipeline, &m_camera.get_view_transformation().data);

#if SHOW_GRID
  // draw grid.
  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, -100, 0);
    ::draw_grid(&pipeline, 5000.f, 100);
    ::pop_matrix(&pipeline);
  }
#endif

  // render the scene.
#if RENDER_SCENE
  if (m_scene) {
   ::push_matrix(&pipeline);
   ::pre_translate(&pipeline, 0, 0, 0);
   ::pre_scale(&pipeline, 1, 1, 1);
   ::draw_meshes(&mesh_render_data[0], &texture_render_id[0], (uint32_t)mesh_render_data.size(), &pipeline);
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

#if 0
    vector3f normals[2];
    get_faces_normals(&faces[0], 2, normals + 0);

    {
      ::push_matrix(&pipeline);
      ::pre_translate(&pipeline, capsules[0].center.data[0], capsules[0].center.data[1], capsules[0].center.data[2]);

      {
        vector3f result;
        segment_t coplanar_overlap;
        capsule_face_classification_t classification =
          classify_capsule_face(&capsules[0], &faces[0], normals, &result, &coplanar_overlap, 1);
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
    }

    {
      ::push_matrix(&pipeline);
      ::pre_translate(&pipeline, capsules[1].center.data[0], capsules[1].center.data[1], capsules[1].center.data[2]);

      {
        vector3f result;
        segment_t coplanar_overlap;
        capsule_face_classification_t classification =
          classify_capsule_face(&capsules[1], &faces[1], normals + 1, &result, &coplanar_overlap, 1);
        if (classification != CAPSULE_FACE_NO_COLLISION) {
          float direction[6];
          float length0 = ::length_v3f(&result);
          float length1 = -(capsules[1].radius - length0);
          result = normalize_v3f(&result);
          direction[0] = result.data[0] * length1;
          direction[1] = result.data[1] * length1;
          direction[2] = result.data[2] * length1;
          direction[3] = direction[0] + result.data[0] * length0;
          direction[4] = direction[1] + result.data[1] * length0;
          direction[5] = direction[2] + result.data[2] * length0;
          ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);

          if (classification == CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE) {
            ::pop_matrix(&pipeline);
            ::draw_lines(coplanar_overlap.points[0].data, 2, color_t{ 0, 1, 0, 1 }, 5, &pipeline);
            ::push_matrix(&pipeline);
            ::pre_translate(&pipeline, capsules[1].center.data[0], capsules[1].center.data[1], capsules[1].center.data[2]);
          }
        }
      }

      ::pop_matrix(&pipeline);
    }

    // draw faces.
    {
      float vertices[12];
      vertices[0 * 3 + 0] = faces[0].points[0].data[0];
      vertices[0 * 3 + 1] = faces[0].points[0].data[1];
      vertices[0 * 3 + 2] = faces[0].points[0].data[2];
      vertices[1 * 3 + 0] = faces[0].points[1].data[0];
      vertices[1 * 3 + 1] = faces[0].points[1].data[1];
      vertices[1 * 3 + 2] = faces[0].points[1].data[2];
      vertices[2 * 3 + 0] = faces[0].points[2].data[0];
      vertices[2 * 3 + 1] = faces[0].points[2].data[1];
      vertices[2 * 3 + 2] = faces[0].points[2].data[2];
      vertices[3 * 3 + 0] = faces[0].points[0].data[0];
      vertices[3 * 3 + 1] = faces[0].points[0].data[1];
      vertices[3 * 3 + 2] = faces[0].points[0].data[2];
      ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);

      vertices[0 * 3 + 0] = faces[1].points[0].data[0];
      vertices[0 * 3 + 1] = faces[1].points[0].data[1];
      vertices[0 * 3 + 2] = faces[1].points[0].data[2];
      vertices[1 * 3 + 0] = faces[1].points[1].data[0];
      vertices[1 * 3 + 1] = faces[1].points[1].data[1];
      vertices[1 * 3 + 2] = faces[1].points[1].data[2];
      vertices[2 * 3 + 0] = faces[1].points[2].data[0];
      vertices[2 * 3 + 1] = faces[1].points[2].data[1];
      vertices[2 * 3 + 2] = faces[1].points[2].data[2];
      vertices[3 * 3 + 0] = faces[1].points[0].data[0];
      vertices[3 * 3 + 1] = faces[1].points[0].data[1];
      vertices[3 * 3 + 2] = faces[1].points[0].data[2];
      ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);
    }

    // draw capsule 00
    {
      uint32_t capsule_texture_render_id[1] = { 0 };
      ::push_matrix(&pipeline);
      ::pre_translate(&pipeline, capsules[0].center.data[0], capsules[0].center.data[1], capsules[0].center.data[2]);
      float scale = (capsules[0].half_height + capsules[0].radius);
      ::pre_scale(&pipeline, scale, scale, scale);
      ::draw_meshes(&collision_render_data[0], capsule_texture_render_id, 1, &pipeline);
      ::pop_matrix(&pipeline);
    }

    // draw capsule 01
    {
      uint32_t capsule_texture_render_id[1] = { 0 };
      ::push_matrix(&pipeline);
      ::pre_translate(&pipeline, capsules[1].center.data[0], capsules[1].center.data[1], capsules[1].center.data[2]);
      float scale = (capsules[1].half_height + capsules[1].radius);
      ::pre_scale(&pipeline, scale, scale, scale);
      ::draw_meshes(&collision_render_data[1], capsule_texture_render_id, 1, &pipeline);
      ::pop_matrix(&pipeline);
    }

    {
      float data[3] = { 2290.51538, -217.184128, -860.228882 };
      ::draw_points(data, 1, { 1.f, 0.f, 0.f, 1.f }, 10, &pipeline);
    }
#endif
  }
#endif

#if OLD_COLLISION_STUFF
  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, sphere_0.center.data[0], sphere_0.center.data[1], sphere_0.center.data[2]);

    {
      vector3f results[5];
      vector3f normals[2];
      memset(results, 0, sizeof(results));
      classify_spheres(&sphere_0, &sphere_1, results + 0);
      get_faces_normals(&face_0, 1, normals + 0);
      get_faces_normals(&face_1, 1, normals + 1);
      classify_sphere_face(&sphere_0, &face_0, normals + 0, results + 1);
      classify_sphere_face(&sphere_0, &face_1, normals + 1, results + 2);
      classify_sphere_capsule(&sphere_0, &capsule_0, results + 3);
      classify_sphere_capsule(&sphere_0, &capsule_1, results + 4);
      for (int32_t i = 0; i < 5; ++i) {
        vector3f result = results[i];
        if (::length_squared_v3f(&result) != 0.f) {
          float direction[6];
          float length0 = ::length_v3f(&result);
          float length1 = -(sphere_0.radius - length0);
          result = normalize_v3f(&result);
          direction[0] = result.data[0] * length1;
          direction[1] = result.data[1] * length1;
          direction[2] = result.data[2] * length1;
          direction[3] = direction[0] + result.data[0] * length0;
          direction[4] = direction[1] + result.data[1] * length0;
          direction[5] = direction[2] + result.data[2] * length0;
          ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);
        }
      }
    }

    ::pop_matrix(&pipeline);
  }

  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, capsule_0.center.data[0], capsule_0.center.data[1], capsule_0.center.data[2]);

    {
      {
        vector3f result; 
        capsules_classification_t classification = classify_capsules(&capsule_0, &capsule_1, &result);

        // draw the axis overlap.
        if (classification != CAPSULES_COLLIDE && classification != CAPSULES_DISTINCT) {
          segment_t debug;
          classify_capsules_segments(&capsule_0, &capsule_1, &debug);

          ::pop_matrix(&pipeline);
          ::draw_points(debug.points[0].data, 2, color_t{ 0, 1, 0.2, 1 }, 10, &pipeline);
          ::draw_lines(debug.points[0].data, 2, color_t{ 0, 1, 0.2, 1 }, 5, &pipeline);
          ::push_matrix(&pipeline);
          ::pre_translate(&pipeline, capsule_0.center.data[0], capsule_0.center.data[1], capsule_0.center.data[2]);
        } else if (::length_squared_v3f(&result) > 0) {
          float direction[6];
          float length0 = ::length_v3f(&result);
          float length1 = -(capsule_0.radius - length0);
          result = normalize_v3f(&result);
          direction[0] = result.data[0] * length1;
          direction[1] = result.data[1] * length1;
          direction[2] = result.data[2] * length1;
          direction[3] = direction[0] + result.data[0] * length0;
          direction[4] = direction[1] + result.data[1] * length0;
          direction[5] = direction[2] + result.data[2] * length0;
          ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);
        }
      }

      {
        vector3f result;
        vector3f normals[2];
        get_faces_normals(&face_0, 1, normals + 0);
        get_faces_normals(&face_1, 1, normals + 1);
        segment_t coplanar_overlap;
        capsule_face_classification_t classification = 
          classify_capsule_face(&capsule_0, &face_0, normals, &result, &coplanar_overlap);
        if (classification != CAPSULE_FACE_NO_COLLISION) {
          float direction[6];
          float length0 = ::length_v3f(&result);
          float length1 = -(capsule_0.radius - length0);
          result = normalize_v3f(&result);
          direction[0] = result.data[0] * length1;
          direction[1] = result.data[1] * length1;
          direction[2] = result.data[2] * length1;
          direction[3] = direction[0] + result.data[0] * length0;
          direction[4] = direction[1] + result.data[1] * length0;
          direction[5] = direction[2] + result.data[2] * length0;
          ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);

          if (classification == CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE) {
            ::pop_matrix(&pipeline);
            ::draw_lines(coplanar_overlap.points[0].data, 2, color_t{ 0, 1, 0, 1 }, 5, &pipeline);
            ::push_matrix(&pipeline);
            ::pre_translate(&pipeline, capsule_0.center.data[0], capsule_0.center.data[1], capsule_0.center.data[2]);
          }
        }
      }

      {
        vector3f result;
        vector3f normals[2];
        get_faces_normals(&face_0, 1, normals + 0);
        get_faces_normals(&face_1, 1, normals + 1);
        segment_t coplanar_overlap;
        capsule_face_classification_t classification = 
          classify_capsule_face(&capsule_0, &face_1, normals + 1, &result, &coplanar_overlap);
        if (classification != CAPSULE_FACE_NO_COLLISION) {
          float direction[6];
          float length0 = ::length_v3f(&result);
          float length1 = -(capsule_0.radius - length0);
          result = normalize_v3f(&result);
          direction[0] = result.data[0] * length1;
          direction[1] = result.data[1] * length1;
          direction[2] = result.data[2] * length1;
          direction[3] = direction[0] + result.data[0] * length0;
          direction[4] = direction[1] + result.data[1] * length0;
          direction[5] = direction[2] + result.data[2] * length0;
          ::draw_lines(direction, 2, color_t{ 0.f, 1.f, 1.f, 1.f }, 2, &pipeline);

          if (classification == CAPSULE_FACE_COLLIDES_CAPSULE_AXIS_COPLANAR_FACE) {
            ::pop_matrix(&pipeline);
            ::draw_lines(coplanar_overlap.points[0].data, 2, color_t{ 0, 1, 0, 1 }, 5, & pipeline);
            ::push_matrix(&pipeline);
            ::pre_translate(&pipeline, capsule_0.center.data[0], capsule_0.center.data[1], capsule_0.center.data[2]);
          }
        }
      }
    }

    ::pop_matrix(&pipeline);
  }

  // draw faces.
  {
    float vertices[12];
    vertices[0 * 3 + 0] = face_0.points[0].data[0];
    vertices[0 * 3 + 1] = face_0.points[0].data[1];
    vertices[0 * 3 + 2] = face_0.points[0].data[2];
    vertices[1 * 3 + 0] = face_0.points[1].data[0];
    vertices[1 * 3 + 1] = face_0.points[1].data[1];
    vertices[1 * 3 + 2] = face_0.points[1].data[2];
    vertices[2 * 3 + 0] = face_0.points[2].data[0];
    vertices[2 * 3 + 1] = face_0.points[2].data[1];
    vertices[2 * 3 + 2] = face_0.points[2].data[2];
    vertices[3 * 3 + 0] = face_0.points[0].data[0];
    vertices[3 * 3 + 1] = face_0.points[0].data[1];
    vertices[3 * 3 + 2] = face_0.points[0].data[2];
    ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);

    vertices[0 * 3 + 0] = face_1.points[0].data[0];
    vertices[0 * 3 + 1] = face_1.points[0].data[1];
    vertices[0 * 3 + 2] = face_1.points[0].data[2];
    vertices[1 * 3 + 0] = face_1.points[1].data[0];
    vertices[1 * 3 + 1] = face_1.points[1].data[1];
    vertices[1 * 3 + 2] = face_1.points[1].data[2];
    vertices[2 * 3 + 0] = face_1.points[2].data[0];
    vertices[2 * 3 + 1] = face_1.points[2].data[1];
    vertices[2 * 3 + 2] = face_1.points[2].data[2];
    vertices[3 * 3 + 0] = face_1.points[0].data[0];
    vertices[3 * 3 + 1] = face_1.points[0].data[1];
    vertices[3 * 3 + 2] = face_1.points[0].data[2];
    ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);
  }

  // draw sphere 00.
  {
    uint32_t sphere_texture_render_id[1] = { 0 };
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, sphere_0.center.data[0], sphere_0.center.data[1], sphere_0.center.data[2]);
    ::pre_scale(&pipeline, sphere_0.radius, sphere_0.radius, sphere_0.radius);
    ::draw_meshes(&collision_render_data[0], sphere_texture_render_id, 1, &pipeline);
    ::pop_matrix(&pipeline);
  }

  // draw sphere 01
  {
    uint32_t sphere_texture_render_id[1] = { 0 };
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, sphere_1.center.data[0], sphere_1.center.data[1], sphere_1.center.data[2]);
    ::pre_scale(&pipeline, sphere_1.radius, sphere_1.radius, sphere_1.radius);
    ::draw_meshes(&collision_render_data[1], sphere_texture_render_id, 1, &pipeline);
    ::pop_matrix(&pipeline);
  }

  // draw capsule 01
  {
    uint32_t capsule_texture_render_id[1] = { 0 };
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, capsule_1.center.data[0], capsule_1.center.data[1], capsule_1.center.data[2]);
    float scale = (capsule_1.half_height + capsule_1.radius);
    ::pre_scale(&pipeline, scale, scale, scale);
    ::draw_meshes(&collision_render_data[3], capsule_texture_render_id, 1, &pipeline);
    ::pop_matrix(&pipeline);
  }

  // draw capsule 00
  {
    uint32_t capsule_texture_render_id[1] = { 0 };
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, capsule_0.center.data[0], capsule_0.center.data[1], capsule_0.center.data[2]);
    // TODO: add a pre_scale uniform.
    float scale = (capsule_0.half_height + capsule_0.radius);
    ::pre_scale(&pipeline, scale, scale, scale);
    ::draw_meshes(&collision_render_data[2], capsule_texture_render_id, 1, &pipeline);
    ::pop_matrix(&pipeline);
  }

  // draw line segments and intersections
  if (1) {
    static float slide_y = 0;
    static float slide_x = 0;
    static float slide_x_delta = 0;
     if (input_static == 2) {
      if (::is_key_pressed('H'))
        slide_x -= 2.f;
      if (::is_key_pressed('K'))
        slide_x += 2.f;
      if (::is_key_pressed('U'))
        slide_y += 2.f;
      if (::is_key_pressed('J'))
        slide_y -= 2.f;
      if (::is_key_pressed('Y'))
        slide_x_delta += 2.f;
      if (::is_key_pressed('I'))
        slide_x_delta -= 2.f;
    }

    segment_t segments[3];
    segments[0].points[0] = { 30 + slide_x, -50 + slide_y, -100 };
    segments[0].points[1] = { 30 + slide_x + slide_x_delta, 0 + slide_y, -100 };
    segments[1].points[0] = { 0, -50, -100 };
    segments[1].points[1] = { 0 - slide_x_delta, 25, -100 };
    ::draw_lines(segments[0].points[0].data, 2, color_t {1, 0, 0, 1}, 1, &pipeline);
    ::draw_lines(segments[1].points[0].data, 2, color_t {0, 0, 1, 1}, 2, &pipeline);
    ::classify_segments(segments + 0, segments + 1, segments + 2);
    ::draw_points(segments[2].points[0].data, 2, color_t{ 0, 1, 0, 1 }, 10, &pipeline);
    ::draw_lines(segments[2].points[0].data, 2, color_t {0, 1, 0, 1}, 5, &pipeline);

    {
      vector3f normals[2];
      get_faces_normals(&face_0, 1, normals + 0);
      get_faces_normals(&face_1, 1, normals + 1);
      float t[4];
      point3f intersection[4];
      segment_plane_classification_t classification[4];
      classification[0] = ::classify_segment_face(&face_0, normals + 0, segments + 0, intersection + 0, t + 0);
      classification[1] = ::classify_segment_face(&face_0, normals + 0, segments + 1, intersection + 1, t + 1);
      classification[2] = ::classify_segment_face(&face_1, normals + 1, segments + 0, intersection + 2, t + 2);
      classification[3] = ::classify_segment_face(&face_1, normals + 1, segments + 1, intersection + 3, t + 3);
      for (int32_t i = 0; i < 4; ++i) {
        if (classification[i] != SEGMENT_PLANE_PARALLEL && classification[i] != SEGMENT_PLANE_COPLANAR) {
          ::draw_points(intersection[i].data, 1, color_t{ 1, 1, 0, 1 }, 10, &pipeline);
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
        vertices[0 * 3 + 0] = collision_faces[i].points[0].data[0];
        vertices[0 * 3 + 1] = collision_faces[i].points[0].data[1];
        vertices[0 * 3 + 2] = collision_faces[i].points[0].data[2];
        vertices[1 * 3 + 0] = collision_faces[i].points[1].data[0];
        vertices[1 * 3 + 1] = collision_faces[i].points[1].data[1];
        vertices[1 * 3 + 2] = collision_faces[i].points[1].data[2];
        vertices[2 * 3 + 0] = collision_faces[i].points[2].data[0];
        vertices[2 * 3 + 1] = collision_faces[i].points[2].data[1];
        vertices[2 * 3 + 2] = collision_faces[i].points[2].data[2];
        vertices[3 * 3 + 0] = collision_faces[i].points[0].data[0];
        vertices[3 * 3 + 1] = collision_faces[i].points[0].data[1];
        vertices[3 * 3 + 2] = collision_faces[i].points[0].data[2];
        ::draw_lines(vertices, 4, { 0.f, 1.f, 0.f, 1.f }, 2, &pipeline);
      }
    }
  }
#endif

  // display simple instructions.
  if (m_font) {
    std::vector<std::string> strvec;
    strvec.push_back("[C] RESET CAMERA");
    strvec.push_back("[~] CAMERA UNLOCK/LOCK");
#if OLD_COLLISION_STUFF
    if (input_static == 0) 
      strvec.push_back("[INPUT] CAPSULE");
    else if (input_static == 1)
      strvec.push_back("[INPUT] SPHERE");
    else if (input_static == 2)
      strvec.push_back("[INPUT] LINE");
#endif

    for (uint32_t i = 0, count = strvec.size(); i < count; ++i) {
      std::vector<unit_quad_t> bounds;
      auto& str = strvec[i];
      for (auto& c : str) {
        auto from = m_font->get_uv_bounds(c);
        unit_quad_t quad;
        quad.data[0] = from[0];
        quad.data[1] = from[1];
        quad.data[2] = from[2];
        quad.data[3] = from[3];
        quad.data[4] = from[4];
        quad.data[5] = from[5];
        bounds.push_back(quad);
      }

      float left, right, bottom, top, nearz, farz;
      float viewport_left, viewport_right, viewport_bottom, viewport_top;
      ::get_frustum(&pipeline, &left, &right, &bottom, &top, &nearz, &farz);
      ::get_viewport_info(&pipeline, &viewport_left, &viewport_top, &viewport_right, &viewport_bottom);

      ::set_orthographic(&pipeline, viewport_left, viewport_right, viewport_top, viewport_bottom, nearz, farz);
      ::update_projection(&pipeline);

      ::push_matrix(&pipeline);
      ::load_identity(&pipeline);
      ::pre_translate(&pipeline, 0, viewport_bottom - ((i + 1) * (float)m_font->get_font_height()), -2);
      ::pre_scale(&pipeline, (float)m_font->get_cell_width(), (float)m_font->get_cell_height(), 0);
      ::draw_unit_quads(&bounds[0], bounds.size(), texture_id[m_font->m_imagefile], &pipeline);
      ::pop_matrix(&pipeline);

      ::set_perspective(&pipeline, left, right, bottom, top, nearz, farz);
      ::update_projection(&pipeline);
    }
  }

  ::flush_operations();

  return controller.end();
}

void 
application::update_camera()
{
  math::matrix4f crossup, camerarotateY, camerarotatetemp, tmpmatrix;
  float dx, dy, d, tempangle;
  static float speed = 10.f;
  float length;
  int32_t mx, my;
  math::vector3f tmp, tmp2;

  if (::is_key_pressed('1')) {
    speed += 0.25;
  }

  if (::is_key_pressed('2')) {
    speed -= 0.25;
    speed = fmax(speed, 0.125);
  }

  if (::is_key_pressed('C')) {
    y_limit = 0;
    m_camera.look_at(math::vector3f(0, 0, 0), math::vector3f(0, 0, -1), math::vector3f(0, 1, 0));
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
  crossup = math::matrix4f::cross_product(m_camera.m_upvector);
  tmp = -m_camera.m_lookatdirection;
  // 'tmp' is now orthogonal to the up and look_at vector.
  tmp = crossup.mult_vec(tmp);

  // Orthogonalizing the m_camera up and direction vector (to avoid floating
  // point imprecision). Discard the y and the w components (preserver x and z,
  // making it parallel to the movement plane).
  tmp.y =/* tmp.w =*/ 0;
  length = sqrtf(tmp.x * tmp.x + tmp.z * tmp.z);
  if (length != 0) {
    // Normalizes tmp and return cross product matrix.
    crossup = math::matrix4f::cross_product(tmp);
    tmp2 = m_camera.m_upvector;
    // 'tmp2' is now the oppposite of the direction vector.
    tmp2 = crossup.mult_vec(tmp2);
    m_camera.m_lookatdirection = -tmp2;
  }

  // Create a rotation matrix on the y relative the movement of the mouse on x.
  camerarotateY = math::matrix4f::rotation_y(-dx / 1000.f);
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
  camerarotatetemp = math::matrix4f::axisangle(tmp, d / K_PI * 180);
  // Switching the rotations here makes no difference, why???, it seems
  // geometrically the result is the same. Just simulate it using your thumb and
  // index.
  tmpmatrix = camerarotateY * camerarotatetemp;
  m_camera.m_lookatdirection = tmpmatrix.mult_vec(m_camera.m_lookatdirection);
  m_camera.m_upvector = tmpmatrix.mult_vec(m_camera.m_upvector);

  // Handling translations.
  if (::is_key_pressed('E')) {
    m_camera.m_position.y -= speed;
  }

  if (::is_key_pressed('Q')) {
    m_camera.m_position.y += speed;
  }

  if (::is_key_pressed('A')) {
    m_camera.m_position.x -= tmp.x * speed;
    m_camera.m_position.z -= tmp.z * speed;
  }

  if (::is_key_pressed('D')) {
    m_camera.m_position.x += tmp.x * speed;
    m_camera.m_position.z += tmp.z * speed;
  }

  static bool k_engaged = false;

  if (::is_key_triggered('N'))
    k_engaged = !k_engaged;

  if (::is_key_pressed('W') || k_engaged) {
    math::vector3f vecxz;
    vecxz.x = m_camera.m_lookatdirection.x;
    vecxz.z = m_camera.m_lookatdirection.z;
    length = sqrtf(vecxz.x * vecxz.x + vecxz.z * vecxz.z);
    if (length != 0) {
      vecxz.x /= length;
      vecxz.z /= length;
      m_camera.m_position.x += vecxz.x * speed;
      m_camera.m_position.z += vecxz.z * speed;
    }
  }

  if (::is_key_pressed('S')) {
    math::vector3f vecxz;
    vecxz.x = m_camera.m_lookatdirection.x;
    vecxz.z = m_camera.m_lookatdirection.z;
    length = sqrtf(vecxz.x * vecxz.x + vecxz.z * vecxz.z);
    if (length != 0) {
      vecxz.x /= length;
      vecxz.z /= length;
      m_camera.m_position.x -= vecxz.x * speed;
      m_camera.m_position.z -= vecxz.z * speed;
    }
  }

#if COLLISION_LOGIC
  {
#if COLLISION_LOGIC_PHYSICS
    if (::is_key_triggered(0x20))
      y_speed = 20.f;

    y_speed -= y_acc;
    y_speed = max(y_speed, -20);
    m_camera.m_position.data.data[1] += y_speed;
#endif

    capsule.center = m_camera.m_position.data;
    vector3f penetration;
    segment_t coplanar_overlap;
    capsule_face_classification_t classification;

    for (uint32_t i = 0; i < collision_faces.size(); ++i) {
      // if the collision face is valid go ahead, otherwise do not bother.
      {
        auto& face = collision_faces[i];
        if (
          ::equal_to_v3f(face.points + 0, face.points + 1) ||
          ::equal_to_v3f(face.points + 0, face.points + 2) ||
          ::equal_to_v3f(face.points + 1, face.points + 2))
          continue;
      }

      capsule_face_classification_t classification = 
        classify_capsule_faceplane(
          &capsule, 
          &collision_faces[i], 
          &collision_normals[i], 
          &penetration);

      if (classification != CAPSULE_FACE_NO_COLLISION) {
        ::add_set_v3f(&capsule.center, &penetration);
        ::add_set_v3f(&m_camera.m_position.data, &penetration);

        // draw the collision faces.
        float vertices[12];
        vertices[0 * 3 + 0] = collision_faces[i].points[0].data[0] + collision_normals[i].data[0] * 1;
        vertices[0 * 3 + 1] = collision_faces[i].points[0].data[1] + collision_normals[i].data[1] * 1;
        vertices[0 * 3 + 2] = collision_faces[i].points[0].data[2] + collision_normals[i].data[2] * 1;
        vertices[1 * 3 + 0] = collision_faces[i].points[1].data[0] + collision_normals[i].data[0] * 1;
        vertices[1 * 3 + 1] = collision_faces[i].points[1].data[1] + collision_normals[i].data[1] * 1;
        vertices[1 * 3 + 2] = collision_faces[i].points[1].data[2] + collision_normals[i].data[2] * 1;
        vertices[2 * 3 + 0] = collision_faces[i].points[2].data[0] + collision_normals[i].data[0] * 1;
        vertices[2 * 3 + 1] = collision_faces[i].points[2].data[1] + collision_normals[i].data[1] * 1;
        vertices[2 * 3 + 2] = collision_faces[i].points[2].data[2] + collision_normals[i].data[2] * 1;
        vertices[3 * 3 + 0] = collision_faces[i].points[0].data[0] + collision_normals[i].data[0] * 1;
        vertices[3 * 3 + 1] = collision_faces[i].points[0].data[1] + collision_normals[i].data[1] * 1;
        vertices[3 * 3 + 2] = collision_faces[i].points[0].data[2] + collision_normals[i].data[2] * 1;
        ::draw_lines(vertices, 4, { 1.f, 0.f, 0.f, 1.f }, 3, &pipeline);
      }
    }
  }
#endif 
}