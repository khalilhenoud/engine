/**
 * @file renderer_opengl.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <renderer/renderer_opengl.h>
#include <renderer/platform/opengl_platform.h>


void
renderer_initialize()
{
  // basic renderer setup.
  float vec[4] = { 1.f, 1.f, 1.f, 1.f };

  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.3f, 0.3f, 0.3f, 1);

  // enable material colors (should be set per mesh).
  glEnable(GL_COLOR_MATERIAL);

  // enabling lighting, setting ambient color, etc...
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, vec);

  // setting the texture blending mode.
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // enabling vertex, normal and UV arrays.
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void
renderer_cleanup()
{
  glBindTexture(GL_TEXTURE_2D, 0);
}

void
clear_color_and_depth_buffers()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
flush_operations()
{
  glFinish();
}

void 
update_viewport(const pipeline_t* pipeline)
{
  float x, y, width, height;
  get_viewport_info(pipeline, &x, &y, &width, &height);

  glViewport((GLint)x, (GLint)y, (GLsizei)width, (GLsizei)height);
}

void 
update_projection(const pipeline_t* pipeline)
{
  float left, right, bottom, top, near_z, far_z;
  get_frustum(pipeline, &left, &right, &bottom, &top, &near_z, &far_z);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (get_projection_type(pipeline) == PERSPECTIVE)
    glFrustum(left, right, bottom, top, near_z, far_z);
  else
    glOrtho(left, right, bottom, top, near_z, far_z);
}

inline
void
set_pipeline_transform(pipeline_t* pipeline)
{
  matrix4f result;
  matrix4f result_column;

  if (pipeline) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    set_matrix_mode(pipeline, MODELVIEW);
    result = get_matrix(pipeline);
    matrix4f_set_column_major(&result_column, &result);
    glMultMatrixf(result_column.data);
  }
}

inline
void
clear_pipeline_transform(pipeline_t* pipeline)
{
  if (pipeline)
    glPopMatrix();
}

void 
draw_grid(
  pipeline_t* pipeline,
  float width,
  int32_t lines_per_axis)
{
  set_pipeline_transform(pipeline);

  glDisable(GL_LIGHTING);
  glColor4f(0, 0, 0, 1);
  glBegin(GL_LINES);
  
  for (int32_t i = 0; i <= lines_per_axis; ++i) {
    glVertex3f(-width / 2, 0, -width / 2 + width / lines_per_axis * i);
    glVertex3f(width / 2, 0, -width / 2 + width / lines_per_axis * i);

    glVertex3f(-width / 2 + width / lines_per_axis * i, 0, -width / 2);
    glVertex3f(-width / 2 + width / lines_per_axis * i, 0, width / 2);
  }
  
  glEnd();
  glEnable(GL_LIGHTING);

  clear_pipeline_transform(pipeline);
}

void
draw_points(
  const float* vertices,
  uint32_t vertices_count,
  color_t color,
  float size,
  pipeline_t* pipeline)
{
  set_pipeline_transform(pipeline);
  
  glDisable(GL_LIGHTING);
  glColor4f(color.data[0], color.data[1], color.data[2], color.data[3]);
  glPointSize(size);
  glBegin(GL_POINTS);

  {
    for (uint32_t i = 0ull; i < vertices_count; ++i)
      glVertex3f(vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]);
  }

  glEnd();
  glPointSize(1.f);
  glEnable(GL_LIGHTING);

  clear_pipeline_transform(pipeline);
}

void
draw_lines(
  const float* vertices,
  uint32_t vertices_count,
  color_t color,
  float width,
  pipeline_t* pipeline)
{
  set_pipeline_transform(pipeline);

  glDisable(GL_LIGHTING);
  glColor4f(color.data[0], color.data[1], color.data[2], color.data[3]);
  glLineWidth(width);
  glBegin(GL_LINES);

  {
    const float* v1 = NULL;
    const float* v2 = NULL;
    const float* v3 = NULL;
    for (uint32_t i = 0ull; i < vertices_count - 1; i++) {
      v1 = vertices + i * 3;
      v2 = vertices + (i + 1) * 3;

      glVertex3f(v1[0], v1[1], v1[2]);
      glVertex3f(v2[0], v2[1], v2[2]);
    }
  }

  glEnd();
  glLineWidth(1.f);
  glEnable(GL_LIGHTING);

  clear_pipeline_transform(pipeline);
}

/// @brief highly specific way to render font.
void
draw_unit_quads(
  const unit_quad_t* uvs,
  uint32_t uvs_count,
  int32_t texture_id,
  pipeline_t* pipeline)
{
  set_pipeline_transform(pipeline);

  if (texture_id) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);
  }

  glDisable(GL_LIGHTING);
  glColor4f(1.f, 1.f, 1.f, 1.f);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

  {
    float left = 0.f;
    float vertices[12] = { 1.f };
    float ogl_uvs[8] = { 0.f };
    uint32_t ogl_idx[6] = { 0, 1, 2, 0, 2, 3 };

    for (uint32_t i = 0; i < uvs_count; ++i) {
      vertices[0] = 0.f + left;
      vertices[1] = uvs[i].data[5];
      vertices[2] = 0.f;

      vertices[3] = 0.f + left;
      vertices[4] = 0.f;
      vertices[5] = 0.f;

      vertices[6] = uvs[i].data[4] + left;
      vertices[7] = 0.f;
      vertices[8] = 0.f;

      vertices[9] = uvs[i].data[4] + left;
      vertices[10] = uvs[i].data[5];
      vertices[11] = 0.f;
      left += uvs[i].data[4];

      ogl_uvs[0] = uvs[i].data[0];
      ogl_uvs[1] = uvs[i].data[1];

      ogl_uvs[2] = uvs[i].data[0];
      ogl_uvs[3] = uvs[i].data[3];

      ogl_uvs[4] = uvs[i].data[2];
      ogl_uvs[5] = uvs[i].data[3];

      ogl_uvs[6] = uvs[i].data[2];
      ogl_uvs[7] = uvs[i].data[1];

      glVertexPointer(3, GL_FLOAT, 0, &vertices[0]);
      glTexCoordPointer(2, GL_FLOAT, 0, &ogl_uvs[0]);
      glDrawElements(GL_TRIANGLES, (GLsizei)6, GL_UNSIGNED_INT, &ogl_idx[0]);
    }
  }
  
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);

  clear_pipeline_transform(pipeline);
}

void
draw_meshes_wireframe(
  const mesh_render_data_t* mesh,
  uint32_t mesh_count,
  color_t color,
  float width,
  pipeline_t* pipeline)
{
  set_pipeline_transform(pipeline);

  glDisable(GL_LIGHTING);
  glColor4f(color.data[0], color.data[1], color.data[2], color.data[3]);
  glLineWidth(width);
  glBegin(GL_LINES);

  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
    const float* v1 = NULL;
    const float* v2 = NULL;
    const float* v3 = NULL;
    for (uint32_t i = 0ull; i < mesh[mesh_index].indices_count; i += 3) {
      v1 = mesh[mesh_index].vertices + mesh[mesh_index].indices[i + 0] * 3;
      v2 = mesh[mesh_index].vertices + mesh[mesh_index].indices[i + 1] * 3;
      v3 = mesh[mesh_index].vertices + mesh[mesh_index].indices[i + 2] * 3;

      glVertex3f(v1[0], v1[1], v1[2]);
      glVertex3f(v2[0], v2[1], v2[2]);
      glVertex3f(v1[0], v1[1], v1[2]);
      glVertex3f(v3[0], v3[1], v3[2]);
      glVertex3f(v2[0], v2[1], v2[2]);
      glVertex3f(v3[0], v3[1], v3[2]);
    }
  }

  glEnd();
  glLineWidth(1.f);
  glEnable(GL_LIGHTING);

  clear_pipeline_transform(pipeline);
}

void 
draw_meshes(
  const mesh_render_data_t* mesh,
  const uint32_t* texture_data,
  uint32_t mesh_count,
  pipeline_t* pipeline)
{
  set_pipeline_transform(pipeline);

  {
    for (uint32_t i = 0; i < mesh_count; ++i) {
      glColorMaterial(GL_FRONT, GL_AMBIENT);
      glColor4f(mesh[i].ambient.data[0], mesh[i].ambient.data[1], mesh[i].ambient.data[2], mesh[i].ambient.data[3]);
      glColorMaterial(GL_FRONT, GL_DIFFUSE);
      glColor4f(mesh[i].diffuse.data[0], mesh[i].diffuse.data[1], mesh[i].diffuse.data[2], mesh[i].diffuse.data[3]);
      glColorMaterial(GL_FRONT, GL_SPECULAR);
      glColor4f(mesh[i].specular.data[0], mesh[i].specular.data[1], mesh[i].specular.data[2], mesh[i].specular.data[3]);

      if (texture_data[i] != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture_data[i]);
      }

      glVertexPointer(3, GL_FLOAT, 0, &mesh[i].vertices[0]);
      glTexCoordPointer(3, GL_FLOAT, 0, &mesh[i].uv_coords[0]);
      glNormalPointer(GL_FLOAT, 0, &mesh[i].normals[0]);
      glDrawElements(GL_TRIANGLES, (GLsizei)mesh[i].indices_count, GL_UNSIGNED_INT, &mesh[i].indices[0]);

      glDisable(GL_TEXTURE_2D);
    }
  }

  clear_pipeline_transform(pipeline);
}

static
uint32_t 
get_component_number(image_format_t format)
{
  switch (format)
  {
  case RENDERER_OPENGL_RGBA:
  case RENDERER_OPENGL_BGRA:
    return 4;
    break;
  case RENDERER_OPENGL_RGB:
  case RENDERER_OPENGL_BGR:
    return 3;
    break;
  case RENDERER_OPENGL_LA:
    return 2;
    break;
  case RENDERER_OPENGL_L:
  case RENDERER_OPENGL_A:
    return 1;
    break;
  }

  assert(0);
  return 0;
}

static
int32_t 
is_component_power_2(image_format_t format)
{
  uint32_t components = get_component_number(format);
  return (components & (components - 1)) == 0;
}

static
GLenum
get_ogl_format(image_format_t format)
{
  switch (format)
  {
  case RENDERER_OPENGL_RGBA:
    return GL_RGBA;
    break;
  case RENDERER_OPENGL_BGRA:
    return GL_BGRA_EXT;
    break;
  case RENDERER_OPENGL_RGB:
    return GL_RGB;
    break;
  case RENDERER_OPENGL_BGR:
    return GL_BGR_EXT;
    break;
  case RENDERER_OPENGL_LA:
    return GL_LUMINANCE_ALPHA;
    break;
  case RENDERER_OPENGL_L:
    return GL_LUMINANCE;
    break;
  case RENDERER_OPENGL_A:
    return GL_ALPHA;
    break;
  default:
    assert(0);
    break;
  }

  return GL_RGBA;
}

uint32_t 
upload_to_gpu(
  const char* path,
  const uint8_t* buffer,
  uint32_t width,
  uint32_t height,
  image_format_t format)
{
  uint32_t n = 0;
  GLenum ogl_format = get_ogl_format(format);

  // upload the texture, generate mipmaps and set wrapping modes.
  glGenTextures(1, &n);
  glBindTexture(GL_TEXTURE_2D, n);
  glPixelStorei(
    GL_UNPACK_ALIGNMENT, 
    is_component_power_2(format) ? get_component_number(format) : 1);
  gluBuild2DMipmaps(
    GL_TEXTURE_2D, 
    get_component_number(format), 
    width, 
    height, 
    ogl_format, 
    GL_UNSIGNED_BYTE, 
    buffer);
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(
    GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  return n;
}

uint32_t
evict_from_gpu(uint32_t texture_id)
{
  glDeleteTextures(1, &texture_id);
  return texture_id;
}