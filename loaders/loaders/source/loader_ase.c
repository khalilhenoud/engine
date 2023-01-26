/**
 * @file loader_ase.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2022-12-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <loaders/utils.h>
#include <loaders/loader_ase.h>


typedef
struct intermediate_face_t {
	uint64_t v0, v1, v2;
	uint64_t v0_t, v1_t, v2_t;
	float normal[3];
  // post processed.
	float v0_normal[3], v1_normal[3], v2_normal[3];
} intermediate_face_t;

typedef
struct normal_additive_t {
  float normal[3];
  uint64_t count;
} normal_additive_t;

void 
process_intermediate_faces(
  uint32_t vertices_number, 
  intermediate_face_t* faces, 
  uint32_t faces_number, 
  const allocator_t* allocator)
{
  float* vertices_normals = (float*)allocator->mem_alloc(vertices_number * 3 * sizeof(float));
  normal_additive_t* shared_normals = (normal_additive_t*)allocator->mem_cont_alloc(vertices_number, sizeof(normal_additive_t));

	for (uint32_t face_index = 0; face_index < faces_number; ++face_index) {
		shared_normals[faces[face_index].v0].normal[0] += faces[face_index].normal[0];
    shared_normals[faces[face_index].v0].normal[1] += faces[face_index].normal[1];
    shared_normals[faces[face_index].v0].normal[2] += faces[face_index].normal[2];
		shared_normals[faces[face_index].v0].count++;
		shared_normals[faces[face_index].v1].normal[0] += faces[face_index].normal[0];
    shared_normals[faces[face_index].v1].normal[1] += faces[face_index].normal[1];
    shared_normals[faces[face_index].v1].normal[2] += faces[face_index].normal[2];
		shared_normals[faces[face_index].v1].count++;
		shared_normals[faces[face_index].v2].normal[0] += faces[face_index].normal[0];
    shared_normals[faces[face_index].v2].normal[1] += faces[face_index].normal[1];
    shared_normals[faces[face_index].v2].normal[2] += faces[face_index].normal[2];
		shared_normals[faces[face_index].v2].count++;
	}

	for (uint32_t i = 0; i < vertices_number; ++i) {
    vertices_normals[i * 3 + 0] = (1.f / (float)shared_normals[i].count) * shared_normals[i].normal[0];
    vertices_normals[i * 3 + 1] = (1.f / (float)shared_normals[i].count) * shared_normals[i].normal[1];
    vertices_normals[i * 3 + 2] = (1.f / (float)shared_normals[i].count) * shared_normals[i].normal[2];
  }

	for (uint32_t face_index = 0; face_index < faces_number; ++face_index) {
		faces[face_index].v0_normal[0] = vertices_normals[faces[face_index].v0 * 3 + 0];
    faces[face_index].v0_normal[1] = vertices_normals[faces[face_index].v0 * 3 + 1];
    faces[face_index].v0_normal[2] = vertices_normals[faces[face_index].v0 * 3 + 2];
		faces[face_index].v1_normal[0] = vertices_normals[faces[face_index].v1 * 3 + 0];
    faces[face_index].v1_normal[1] = vertices_normals[faces[face_index].v1 * 3 + 1];
    faces[face_index].v1_normal[2] = vertices_normals[faces[face_index].v1 * 3 + 2];
		faces[face_index].v2_normal[0] = vertices_normals[faces[face_index].v2 * 3 + 0];
    faces[face_index].v2_normal[1] = vertices_normals[faces[face_index].v2 * 3 + 1];
    faces[face_index].v2_normal[2] = vertices_normals[faces[face_index].v2 * 3 + 2];
	}

  allocator->mem_free(vertices_normals);
  allocator->mem_free(shared_normals);
}

typedef
struct intermediate_mesh_t {
	fixed_str_t name;
	float* vertices;
  uint32_t vertices_count;
	float* texture_vertices;
  uint32_t texture_vertices_count;
	intermediate_face_t* faces;
  uint32_t faces_count;
	uint32_t material_index;
} intermediate_mesh_t;

intermediate_mesh_t*
allocate_intermediate_mesh(
  uint32_t vertices_count, 
  uint32_t texture_vertices_count, 
  uint32_t faces_count, 
  const allocator_t* allocator)
{
  intermediate_mesh_t* mesh = (intermediate_mesh_t *)allocator->mem_alloc(sizeof(intermediate_mesh_t));
  mesh->vertices_count = vertices_count;
  mesh->vertices = (float *)allocator->mem_alloc(sizeof(float) * mesh->vertices_count * 3);
  mesh->texture_vertices_count = texture_vertices_count;
  mesh->texture_vertices = (float *)allocator->mem_alloc(sizeof(float) * mesh->texture_vertices_count * 3);
  mesh->faces_count = faces_count;
  mesh->faces = (intermediate_face_t *)allocator->mem_cont_alloc(mesh->faces_count, sizeof(intermediate_face_t));
  return mesh;
}

void
free_intermediate_mesh(
  intermediate_mesh_t* mesh,
  const allocator_t* allocator)
{
  allocator->mem_free(mesh->vertices);
  allocator->mem_free(mesh->texture_vertices);
  allocator->mem_free(mesh->faces);
  allocator->mem_free(mesh);
}

void
convert_intermediate_mesh(
  loader_mesh_data_t* mesh_data,
  const intermediate_mesh_t* mesh, 
  const allocator_t* allocator)
{
  sprintf(mesh_data->name.data, "%s", mesh->name.data);
  mesh_data->vertices_count = mesh->texture_vertices_count;
  mesh_data->vertices = (float *)allocator->mem_alloc(mesh->texture_vertices_count * 3 * sizeof(float));
  mesh_data->normals = (float *)allocator->mem_alloc(mesh->texture_vertices_count * 3 * sizeof(float));
  mesh_data->uvs = (float *)allocator->mem_alloc(mesh->texture_vertices_count * 3 * sizeof(float));
  mesh_data->faces_count = mesh->faces_count;
  mesh_data->indices = (uint32_t *)allocator->mem_alloc(mesh->faces_count * 3 * sizeof(uint32_t));
  mesh_data->materials.indices[mesh_data->materials.used++] = mesh->material_index;

  for (uint32_t i = 0, index = 0; i < mesh->faces_count; ++i) {
    intermediate_face_t* face = mesh->faces + i;
    mesh_data->indices[index++] = (uint32_t)face->v0_t;
    mesh_data->indices[index++] = (uint32_t)face->v1_t;
    mesh_data->indices[index++] = (uint32_t)face->v2_t;

    mesh_data->vertices[face->v0_t * 3 + 0] = mesh->vertices[face->v0 * 3 + 0];
    mesh_data->vertices[face->v0_t * 3 + 1] = mesh->vertices[face->v0 * 3 + 1];
    mesh_data->vertices[face->v0_t * 3 + 2] = mesh->vertices[face->v0 * 3 + 2];

    mesh_data->vertices[face->v1_t * 3 + 0] = mesh->vertices[face->v1 * 3 + 0];
    mesh_data->vertices[face->v1_t * 3 + 1] = mesh->vertices[face->v1 * 3 + 1];
    mesh_data->vertices[face->v1_t * 3 + 2] = mesh->vertices[face->v1 * 3 + 2];

    mesh_data->vertices[face->v2_t * 3 + 0] = mesh->vertices[face->v2 * 3 + 0];
    mesh_data->vertices[face->v2_t * 3 + 1] = mesh->vertices[face->v2 * 3 + 1];
    mesh_data->vertices[face->v2_t * 3 + 2] = mesh->vertices[face->v2 * 3 + 2];

    mesh_data->normals[face->v0_t * 3 + 0] = face->v0_normal[0];
    mesh_data->normals[face->v0_t * 3 + 1] = face->v0_normal[1];
    mesh_data->normals[face->v0_t * 3 + 2] = face->v0_normal[2];

    mesh_data->normals[face->v1_t * 3 + 0] = face->v1_normal[0];
    mesh_data->normals[face->v1_t * 3 + 1] = face->v1_normal[1];
    mesh_data->normals[face->v1_t * 3 + 2] = face->v1_normal[2];

    mesh_data->normals[face->v2_t * 3 + 0] = face->v2_normal[0];
    mesh_data->normals[face->v2_t * 3 + 1] = face->v2_normal[1];
    mesh_data->normals[face->v2_t * 3 + 2] = face->v2_normal[2];

    mesh_data->uvs[face->v0_t * 3 + 0] = mesh->texture_vertices[face->v0_t * 3 + 0];
    mesh_data->uvs[face->v0_t * 3 + 1] = mesh->texture_vertices[face->v0_t * 3 + 1];
    mesh_data->uvs[face->v0_t * 3 + 2] = mesh->texture_vertices[face->v0_t * 3 + 2];

    mesh_data->uvs[face->v1_t * 3 + 0] = mesh->texture_vertices[face->v1_t * 3 + 0];
    mesh_data->uvs[face->v1_t * 3 + 1] = mesh->texture_vertices[face->v1_t * 3 + 1];
    mesh_data->uvs[face->v1_t * 3 + 2] = mesh->texture_vertices[face->v1_t * 3 + 2];

    mesh_data->uvs[face->v2_t * 3 + 0] = mesh->texture_vertices[face->v2_t * 3 + 0];
    mesh_data->uvs[face->v2_t * 3 + 1] = mesh->texture_vertices[face->v2_t * 3 + 1];
    mesh_data->uvs[face->v2_t * 3 + 2] = mesh->texture_vertices[face->v2_t * 3 + 2];
  }
}


typedef 
struct chunk_t {
  const char* start;
  size_t size;
} chunk_t;

chunk_t 
read_chunk(
  const chunk_t *chunk, 
  const char *label)
{
  chunk_t result;
  const char* start = NULL;
  start = strstr(chunk->start, label);
  assert(start);

  const char *open_bracket = strchr(start, '{');
  assert(open_bracket);

  int32_t count = 1;
	char ch;
  const char *end_bracket = open_bracket++;
	while (count) {
		++end_bracket;
    end_bracket = strpbrk(end_bracket, "{}");
		ch = *end_bracket;

		if (ch == '{')
			++count;
		if (ch == '}')
			--count;
	}

  assert(end_bracket <= (chunk->start + chunk->size));

  result.start = open_bracket;
  result.size = (size_t)(end_bracket - open_bracket);
  return result;
}

void
read_color(
  const chunk_t *pchunk,
  const char* label,
  loader_color_data_t* color)
{
  color->data[0] = color->data[1] = color->data[2] = color->data[3] = 1.f;
  const char* color_start = strstr(pchunk->start, label);
  if (!color_start)
    return;

  color_start += strlen(label);
  sscanf(color_start, " %f %f %f", color->data + 0, color->data + 1, color->data + 2);
}

void
read_float(
  const chunk_t *pchunk,
  const char* label,
  float* value)
{
  *value = 0.f;
  const char* value_start = strstr(pchunk->start, label);
  if (!value_start)
    return;

  value_start += strlen(label);
  sscanf(value_start, " %f", value);
}

void
read_uint32_t(
  const chunk_t *pchunk,
  const char* label,
  uint32_t* value)
{
  *value = 0u;
  const char* value_start = strstr(pchunk->start, label);
  if (!value_start)
    return;

  value_start += strlen(label);
  sscanf(value_start, " %u", value);
}

void
read_string_512(
  const chunk_t *pchunk, 
  const char *label, 
  char *data)
{  
  const char *name = strstr(pchunk->start, label);
  assert(name);
  name += strlen(label);
  name = strchr(name, '\"');
  assert(name);
  name++;
  const char *name_end = strchr(name, '\"');
  int32_t name_length = (int32_t)(name_end - name);
  assert(name_length < 512);
  memset(data, 0, 512);
  sprintf(data, "%.*s", name_length, name);
}

int32_t 
has_label(
  const chunk_t *pchunk, 
  const char* label)
{
  const char* content = strstr(pchunk->start, label);
  if (!content || content > (pchunk->start + pchunk->size))
    return 0;
  return 1;
}

void 
within_chunk(
  const chunk_t *pchunk,
  void *ptr)
{
  assert(ptr >= pchunk->start && ptr <= (pchunk->start + pchunk->size));
}

void 
read_texture(
  const chunk_t *pchunk, 
  const char* label,
  loader_texture_data_t* data)
{
  chunk_t chunk = read_chunk(pchunk, label);
  read_string_512(&chunk, "*MAP_NAME", data->name.data);
  read_string_512(&chunk, "*MAP_CLASS", data->type.data);
  read_string_512(&chunk, "*BITMAP", data->path.data);
  read_float(&chunk, "*UVW_U_OFFSET", &data->u);
  read_float(&chunk, "*UVW_V_OFFSET", &data->v);
  read_float(&chunk, "*UVW_U_TILING", &data->u_scale);
  read_float(&chunk, "*UVW_V_TILING", &data->v_scale);
  read_float(&chunk, "*UVW_ANGLE", &data->angle);
}

void 
read_ase_materials(
  const chunk_t *pchunk, 
  loader_material_repo_t *material_repo,
  const allocator_t *allocator)
{
  chunk_t chunk = read_chunk(pchunk, "*MATERIAL_LIST");

  const char *target = strstr(chunk.start, "*MATERIAL_COUNT");
  assert(target);
  target += strlen("*MATERIAL_COUNT");

  int32_t material_count = 0;
  sscanf(target, " %d", &material_count);
  material_repo->used = (uint32_t)material_count;
  material_repo->data = (loader_material_data_t *)allocator->mem_cont_alloc((size_t)material_count, sizeof(loader_material_data_t));

  {
    loader_material_data_t* material = NULL;
    char target_label[512] = { 0 };
    chunk_t subchunk;

    // Reading the materials.
    for (int32_t i = 0; i < material_count; ++i) {
      sprintf(target_label, "*MATERIAL %d", i);
      material = material_repo->data + i;
      assert(material);

      subchunk = read_chunk(&chunk, target_label);
      read_string_512(&subchunk, "*MATERIAL_NAME", material->name.data);

      read_color(&subchunk, "*MATERIAL_AMBIENT", &material->ambient);
      read_color(&subchunk, "*MATERIAL_DIFFUSE", &material->diffuse);
      read_color(&subchunk, "*MATERIAL_SPECULAR", &material->specular);

      read_float(&subchunk, "*MATERIAL_SHINESTRENGTH", &material->shininess);
      read_float(&subchunk, "*MATERIAL_TRANSPARENCY", &material->opacity);
      material->opacity = 1.f - material->opacity;

      if (has_label(&subchunk, "*MAP_DIFFUSE"))
        read_texture(&subchunk, "*MAP_DIFFUSE", &material->textures.data[material->textures.used++]);
      if (has_label(&subchunk, "*MAP_OPACITY"))
        read_texture(&subchunk, "*MAP_OPACITY", &material->textures.data[material->textures.used++]);
      if (has_label(&subchunk, "*MAP_BUMP"))
        read_texture(&subchunk, "*MAP_OPACITY", &material->textures.data[material->textures.used++]);
    }
  }
}

loader_ase_data_t* 
read_scene(
  const chunk_t *pchunk, 
  const allocator_t* allocator)
{
  loader_ase_data_t* scene_data = (loader_ase_data_t *)allocator->mem_alloc(sizeof(loader_ase_data_t));
  memset(scene_data, 0, sizeof(loader_ase_data_t));

  read_ase_materials(pchunk, &scene_data->material_repo, allocator);

  // count the number of meshes/models.
  uint32_t mesh_count = 0;
  {
    const char* ptr = strstr(pchunk->start, "*GEOMOBJECT");
    while (ptr) {
      ++mesh_count;
      ptr += strlen("*GEOMOBJECT");
      ptr = strstr(ptr, "*GEOMOBJECT");
    }
  }
  
  // 1 to 1 with models.
  scene_data->mesh_repo.used = mesh_count;
  scene_data->mesh_repo.data = (loader_mesh_data_t *)allocator->mem_cont_alloc(mesh_count, sizeof(loader_mesh_data_t));

  scene_data->model_repo.used = mesh_count;
  scene_data->model_repo.data = (loader_model_data_t *)allocator->mem_cont_alloc(mesh_count, sizeof(loader_model_data_t));

  uint32_t index = 0;
  const char* current = strstr(pchunk->start, "*GEOMOBJECT");

	while (current) {
    chunk_t tmpchunk = { current, pchunk->size - (current - pchunk->start) };
    chunk_t chunk = read_chunk(&tmpchunk, "*GEOMOBJECT");
    loader_mesh_data_t* loader_mesh = scene_data->mesh_repo.data + index;
    loader_model_data_t* model = scene_data->model_repo.data + index;
    model->meshes.indices[model->meshes.used++] = index;

    uint32_t i_m_count[3];
    read_uint32_t(&chunk, "*MESH_NUMVERTEX", i_m_count + 0);
    read_uint32_t(&chunk, "*MESH_NUMTVERTEX", i_m_count + 1);
    read_uint32_t(&chunk, "*MESH_NUMFACES", i_m_count + 2);

    intermediate_mesh_t* mesh = allocate_intermediate_mesh(
      i_m_count[0], 
      i_m_count[1], 
      i_m_count[2], 
      allocator);
    read_string_512(&chunk, "*NODE_NAME", model->name.data);
		sprintf(mesh->name.data, "%s_mesh", model->name.data);
    read_uint32_t(&chunk, "*MATERIAL_REF", &mesh->material_index);

    {
      chunk_t subchunk = read_chunk(&chunk, "*MESH_VERTEX_LIST");
      const char* ptr = strstr(subchunk.start, "*MESH_VERTEX");
      for (uint32_t i = 0; i < mesh->vertices_count; ++i) {
        sscanf(ptr, 
          "*MESH_VERTEX %*i %f %f %f", 
          &mesh->vertices[i * 3 + 0], 
          &mesh->vertices[i * 3 + 1],
          &mesh->vertices[i * 3 + 2]);
        ptr += strlen("*MESH_VERTEX");
        ptr = strstr(ptr, "*MESH_VERTEX");
      }
    }

    {
      chunk_t subchunk = read_chunk(&chunk, "*MESH_TVERTLIST");
      const char* ptr = strstr(subchunk.start, "*MESH_TVERT");
      for (uint32_t i = 0; i < mesh->texture_vertices_count; ++i) {
        sscanf(ptr, 
          "*MESH_TVERT %*i %f %f %f", 
          &mesh->texture_vertices[i * 3 + 0], 
          &mesh->texture_vertices[i * 3 + 1],
          &mesh->texture_vertices[i * 3 + 2]);
        ptr += strlen("*MESH_TVERT");
        ptr = strstr(ptr, "*MESH_TVERT");
      }
    }

    {
      chunk_t subchunk = read_chunk(&chunk, "*MESH_FACE_LIST");
      const char* ptr = strstr(subchunk.start, "*MESH_FACE");
      for (uint32_t i = 0; i < mesh->faces_count; ++i) {
        sscanf(ptr,
          "*MESH_FACE %*i: A: %llu B: %llu C: %llu",
          &mesh->faces[i].v0,
          &mesh->faces[i].v1,
          &mesh->faces[i].v2);
        ptr += strlen("*MESH_FACE");
        ptr = strstr(ptr, "*MESH_FACE");
      }
    }

    {
      chunk_t subchunk = read_chunk(&chunk, "*MESH_TFACELIST");
      const char* ptr = strstr(subchunk.start, "*MESH_TFACE");
      for (uint32_t i = 0; i < mesh->faces_count; ++i) {
        sscanf(ptr,
          "*MESH_TFACE %*i %llu %llu %llu",
          &mesh->faces[i].v0_t,
          &mesh->faces[i].v1_t,
          &mesh->faces[i].v2_t);
        ptr += strlen("*MESH_TFACE");
        ptr = strstr(ptr, "*MESH_TFACE");
      }
    }

    {
      chunk_t subchunk = read_chunk(&chunk, "*MESH_NORMALS");
      const char* ptr = strstr(subchunk.start, "*MESH_FACENORMAL");
      for (uint32_t i = 0; i < mesh->faces_count; ++i) {
        sscanf(ptr,
          "*MESH_FACENORMAL %*i %f %f %f",
          &mesh->faces[i].normal[0],
          &mesh->faces[i].normal[1],
          &mesh->faces[i].normal[2]);
        ptr += strlen("*MESH_FACENORMAL");
        ptr = strstr(ptr, "*MESH_FACENORMAL");
      }
    }

    process_intermediate_faces(
      mesh->vertices_count, 
      mesh->faces, 
      mesh->faces_count,
      allocator);

    convert_intermediate_mesh(
      loader_mesh, 
      mesh, 
      allocator);

    free_intermediate_mesh(mesh, allocator);

    ++index;
    current += strlen("*GEOMOBJECT");
    current = strstr(current, "*GEOMOBJECT");
	}

	return scene_data;
}

loader_ase_data_t* 
load_ase(
  const char* path, 
  const allocator_t *allocator)
{
  size_t file_size = 0;
  const char* data = read_file_as_ascii(path, allocator, &file_size);
  chunk_t chunk = { data, file_size };
	loader_ase_data_t* scene_data = read_scene(&chunk, allocator);
  allocator->mem_free((void*)data);
  return scene_data;
}

void
free_ase(
  loader_ase_data_t* data, 
  const allocator_t *allocator)
{
  allocator->mem_free(data->material_repo.data);
  allocator->mem_free(data->model_repo.data);
  for (uint32_t i = 0; i < data->mesh_repo.used; ++i) {
    loader_mesh_data_t* mesh = data->mesh_repo.data + i;
    allocator->mem_free(mesh->vertices);
    allocator->mem_free(mesh->normals);
    allocator->mem_free(mesh->uvs);
    allocator->mem_free(mesh->indices);
  }
  allocator->mem_free(data->mesh_repo.data);
  allocator->mem_free(data);
}