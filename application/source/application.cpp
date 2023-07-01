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
#include <math/vector3f.h>
#include <renderer/renderer_opengl.h>
#include <renderer/pipeline.h>
#include <loaders/loader_ase.h>
#include <loaders/loader_png.h>
#include <loaders/loader_csv.h>
#include <data_format/mesh_format/data_format.h>
#include <data_format/mesh_format/utils.h>
#include <collision/capsule.h>
#include <collision/segment.h>
#include <collision/face.h>
#include <collision/sphere.h>


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
static 
bool
load_image(
  std::string& dataset, 
  entity::image& image, 
  const allocator_t* allocator)
{
  if (image.get_extension() != ".png")
    assert(false && "image type not supported!");

  auto fullpath = dataset + image.m_path;
  loader_png_data_t* data = load_png(fullpath.c_str(), allocator);
  assert(data);

  image.m_buffer.insert(
    std::end(image.m_buffer), 
    &data->buffer[0], 
    &data->buffer[data->total_buffer_size]);
  image.m_width = data->width;
  image.m_height = data->height;
  image.m_format = static_cast<entity::image::format>(data->format);

  free_png(data, allocator);
  return true;
}

static 
std::unique_ptr<entity::node> 
load_ase_model(
  std::string& dataset, 
  std::string& path, 
  const allocator_t* allocator)
{
  auto fullpath = dataset + path;
  loader_ase_data_t* ase_data = load_ase(fullpath.c_str(), allocator);
  std::unique_ptr<entity::node> entity_model = std::make_unique<entity::node>();

  auto copy_textures = 
  [&](loader_texture_data_t& source, entity::texture& target) {
      target.m_name = source.name.data;
      target.m_path = source.path.data;
      target.m_type = source.type.data;
      target.m_u = source.u;
      target.m_v = source.v;
      target.m_u_scale = source.u_scale;
      target.m_v_scale = source.v_scale;
      target.m_angle = source.angle;
  };

  auto copy_materials = 
  [&](loader_ase_data_t* source, uint32_t material_index, entity::material& target) {
    assert(material_index < source->material_repo.used);
    auto material_data = source->material_repo.data[material_index];
    target.m_name = material_data.name.data;
    target.m_ambient = { 
      material_data.ambient.data[0], 
      material_data.ambient.data[1], 
      material_data.ambient.data[2], 
      material_data.ambient.data[3] };
    target.m_diffuse = { 
      material_data.diffuse.data[0], 
      material_data.diffuse.data[1], 
      material_data.diffuse.data[2], 
      material_data.diffuse.data[3] };
    target.m_specular = { 
      material_data.specular.data[0], 
      material_data.specular.data[1], 
      material_data.specular.data[2], 
      material_data.specular.data[3] };
    target.m_opacity = material_data.opacity;
    target.m_shininess = material_data.shininess;
    for (uint32_t i = 0; i < material_data.textures.used; ++i) {
      target.m_textures.push_back(entity::texture());
      copy_textures(material_data.textures.data[i], target.m_textures.back());
    }
  };

  auto copy_meshes = 
  [&](loader_ase_data_t* source, uint32_t mesh_index, entity::node* target) {
    assert(mesh_index < source->mesh_repo.used);
    auto mesh_data = source->mesh_repo.data[mesh_index];
    auto target_mesh = target->m_meshes.emplace_back(std::make_shared<entity::mesh>()).get();
    target_mesh->m_name = mesh_data.name.data;
    target_mesh->m_vertices.insert(
      target_mesh->m_vertices.end(), 
      &mesh_data.vertices[0],
      &mesh_data.vertices[mesh_data.vertices_count * 3]);
    target_mesh->m_normals.insert(
      target_mesh->m_normals.end(), 
      &mesh_data.normals[0],
      &mesh_data.normals[mesh_data.vertices_count * 3]);
    target_mesh->m_uvCoords.emplace_back(std::vector<float>());
    target_mesh->m_uvCoords[0].insert(
      target_mesh->m_uvCoords[0].end(), 
      &mesh_data.uvs[0],
      &mesh_data.uvs[mesh_data.vertices_count * 3]);
    target_mesh->m_indices.insert(
      target_mesh->m_indices.end(), 
      &mesh_data.indices[0],
      &mesh_data.indices[mesh_data.faces_count * 3]);
    for (uint32_t i = 0; i < mesh_data.materials.used; ++i) {
      uint32_t material_id = mesh_data.materials.indices[i];
      assert(material_id < source->material_repo.used);
      target_mesh->m_materials.push_back(entity::material());
      copy_materials(source, material_id, target_mesh->m_materials.back());
    }
  };

  using ftype = std::function<void(loader_ase_data_t* source, uint32_t model_index, entity::node*)>;
  ftype copy_models = 
  [&](loader_ase_data_t* source, uint32_t model_index, entity::node* target) {
    // this always starts with model_index = 0;
    assert(model_index < source->model_repo.used);
    auto model_data = source->model_repo.data[model_index];
    
    target->m_name = model_data.name.data;

    for (uint32_t i = 0; i < model_data.meshes.used; ++i) {
      uint32_t mesh_index = model_data.meshes.indices[i];
      assert(mesh_index < source->mesh_repo.used);
      copy_meshes(
        source, 
        mesh_index, 
        target);
    }

    for (uint32_t i = 0; i < model_data.models.used; ++i) {
      uint32_t model_index = model_data.models.indices[i];
      assert(model_index < source->model_repo.used);
      copy_models(
        source, 
        model_index, 
        target->m_children.emplace_back(std::make_shared<entity::node>()).get());
    }
  };

  for (uint32_t i = 0; i < ase_data->model_repo.used; ++i) {
    copy_models(
      ase_data, 
      i, 
      entity_model->m_children.emplace_back(std::make_shared<entity::node>()).get());
  }
  
  free_ase(ase_data, allocator);
  return entity_model;
}

static 
std::pair<std::vector<mesh_render_data_t>, std::vector<const char*>>
load_renderer_data(entity::node& model)
{
  std::vector<mesh_render_data_t> render_meshes;
  std::vector<const char*> texture_paths;
  auto entity_meshes = model.get_meshes();

  for (auto& mesh : entity_meshes)
  {
    if (mesh->m_materials.size())
    {
      auto& material = mesh->m_materials[0];
      mesh_render_data_t render_mesh = {
        &mesh->m_vertices[0], 
        &mesh->m_normals[0], 
        &(mesh->m_uvCoords[0])[0],
        (uint32_t)mesh->m_vertices.size()/3,
        &mesh->m_indices[0],
        (uint32_t)mesh->m_indices.size(),
        { material.m_ambient[0], material.m_ambient[1], material.m_ambient[2], material.m_ambient[3] },
        { material.m_diffuse[0], material.m_diffuse[1], material.m_diffuse[2], material.m_diffuse[3] },
        { material.m_specular[0], material.m_specular[1], material.m_specular[2], material.m_specular[3] }
      };
      render_meshes.emplace_back(render_mesh);
      texture_paths.push_back(material.m_textures.size() ? material.m_textures[0].m_path.c_str() : nullptr);
    }
    else
    {
      mesh_render_data_t render_mesh = {
        &mesh->m_vertices[0], 
        &mesh->m_normals[0], 
        &(mesh->m_uvCoords[0])[0],
        (uint32_t)mesh->m_vertices.size()/3,
        &mesh->m_indices[0],
        (uint32_t)mesh->m_indices.size(),
      };
      render_meshes.emplace_back(render_mesh);
      texture_paths.push_back(nullptr);
    }
  }

  return std::make_pair(render_meshes, texture_paths);
}

static
std::unique_ptr<entity::font>
load_font(
  std::string& data_set,
  std::string& image_file, 
  std::string& data_file, 
  const allocator_t* allocator)
{
  auto file = data_set + data_file;
  loader_csv_font_data_t* data_font = load_csv(file.c_str(), allocator);
  std::unique_ptr<entity::font> font = std::make_unique<entity::font>(image_file, data_file);
  font->m_imagewidth  = data_font->image_width;
  font->m_imageheight = data_font->image_height;
  font->m_cellwidth   = data_font->cell_width;
  font->m_cellheight  = data_font->cell_height;
  font->m_fontheight  = data_font->font_height;
  font->m_fontwidth   = data_font->font_width;
  font->m_startchar   = data_font->start_char;

  for (uint32_t i = 0; i < entity::font::s_gliphcount; ++i)
  {
    font->m_glyphs[i] = { 
      data_font->glyphs[i].x, 
      data_font->glyphs[i].y,
      data_font->glyphs[i].width,
      data_font->glyphs[i].offset};

    font->m_bounds[i][0] = data_font->bounds[i].data[0];
    font->m_bounds[i][1] = data_font->bounds[i].data[1];
    font->m_bounds[i][2] = data_font->bounds[i].data[2];
    font->m_bounds[i][3] = data_font->bounds[i].data[3];
    font->m_bounds[i][4] = data_font->bounds[i].data[4];
    font->m_bounds[i][5] = data_font->bounds[i].data[5];
  }

  free_csv(data_font, allocator);
  return font;
}


////////////////////////////////////////////////////////////////////////////////
static 
mesh_render_data_t
load_mesh_renderer_data(mesh_t* mesh, color_t color)
{
  mesh_render_data_t render_mesh = {
    &mesh->vertices[0], 
    &mesh->normals[0], 
    &mesh->uvs[0],
    mesh->vertices_count,
    &mesh->indices[0],
    mesh->indices_count,
    color,
    color,
    color
  };

  return render_mesh;
}


////////////////////////////////////////////////////////////////////////////////
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
mesh_t* meshes[4] = { nullptr };
std::vector<mesh_render_data_t> collision_render_data;
sphere_t sphere_0, sphere_1;
capsule_t capsule_0, capsule_1;
face_t face_0, face_1;
allocator_t allocator;

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
    // TODO: This needs fixing (the scaling is off).
    meshes[3] = ::create_unit_capsule(30, capsule_1.half_height / capsule_1.radius, &allocator);
    collision_render_data.push_back(load_mesh_renderer_data(meshes[3], color_t{ 1.f, 0.f, 1.f, 0.5f }));

    face_0.points[0] = { 0.f, -100.f, -200.f};
    face_0.points[1] = { 50.f, -100.f, -200.f};
    face_0.points[2] = { 50.f, -100.f, -250.f};

    face_1.points[0] = { 200.f, -100.f, -200.f};
    face_1.points[1] = { 200.f, -50.f, -200.f};
    face_1.points[2] = { 200.f, -50.f, -250.f};
  }

  auto datafile = std::string("media\\font\\FontData2.csv");
  auto imagefile = std::string("media\\font\\ExportedFont2.png");
  m_font = load_font(m_dataset, imagefile, datafile, &allocator);
  m_images.emplace_back(imagefile);

  auto model_path = std::string("media\\test\\test01.ASE");
 /* m_scene = load_ase_model(m_dataset, model_path, &allocator);
  std::tie(mesh_render_data, texture_render_data) = load_renderer_data(*m_scene);

  for (auto& entry : m_scene->get_textures_paths())
    m_images.emplace_back(entry.c_str());*/

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
    if (texture_id.find(std::string(path)) != texture_id.end()) 
      texture_render_id.push_back(texture_id[std::string(path)]);
    else
      texture_render_id.push_back(0);
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
  ::free_mesh(meshes[0], &allocator);
  ::free_mesh(meshes[1], &allocator);
  ::free_mesh(meshes[2], &allocator);
  ::free_mesh(meshes[3], &allocator);

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
  }

  ::set_matrix_mode(&pipeline, MODELVIEW);
  ::load_identity(&pipeline);
  ::post_multiply(&pipeline, &m_camera.get_view_transformation().data);

  // draw grid.
  {
    ::push_matrix(&pipeline);
    ::pre_translate(&pipeline, 0, -100, 0);
    ::draw_grid(&pipeline, 5000.f, 100);
    ::pop_matrix(&pipeline);
  }

  // render the scene.
  // if (m_scene) {
  //  ::push_matrix(&pipeline);
  //  ::pre_translate(&pipeline, 0, -120, -300);
  //  ::pre_scale(&pipeline, 3, 3, 3);
  //  ::draw_meshes(&mesh_render_data[0], &texture_render_id[0], (uint32_t)mesh_render_data.size(), &pipeline);
  //  ::pop_matrix(&pipeline);
  // }

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

  // display simple instructions.
  if (m_font) {
    std::vector<std::string> strvec;
    strvec.push_back("[C] RESET CAMERA");
    strvec.push_back("[~] CAMERA UNLOCK/LOCK");
    if (input_static == 0) 
      strvec.push_back("[INPUT] CAPSULE");
    else if (input_static == 1)
      strvec.push_back("[INPUT] SPHERE");
    else if (input_static == 2)
      strvec.push_back("[INPUT] LINE");

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
  float speed = 10.f;
  float length;
  int32_t mx, my;
  math::vector3f tmp, tmp2;

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

  if (::is_key_pressed('W')) {
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
}