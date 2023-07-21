/**
 * @file mesh.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <string>
#include <vector>
#include <array>
#include <cstdint>


namespace entity {

////////////////////////////////////////////////////////////////////////////////
struct 
texture {
  std::string m_name;
  std::string m_path;
  std::string m_type;               // i.e. diffuse, specular, etc...
  float m_u = 0.f, m_v = 0.f;
  float m_u_scale = 0.f, m_v_scale = 0.f;
  float m_angle = 0.f;
};

using color = std::array<float, 4>;

struct 
material {
  std::string m_name;
  color m_ambient;
  color m_diffuse;
  color m_specular;
  float m_shininess = 0.f;
  float m_opacity = 1.f;
  // maximum 8.
  std::vector<texture> m_textures;
};

////////////////////////////////////////////////////////////////////////////////
struct 
mesh {

  std::vector<std::string> 
  get_textures_paths()
  {
    std::vector<std::string> paths;
    for (uint32_t i = 0; i < m_materials.size(); ++i) {
      for (uint32_t j = 0; j < m_materials[i].m_textures.size(); ++j)
        paths.push_back(m_materials[i].m_textures[i].m_path);
    }

    return paths;
  }

  //////////////////////////////////////////////////////////////////////////////
  std::string m_name;
  std::vector<float> m_vertices;  // 3 floats
  std::vector<float> m_normals;   // 3 floats
  std::vector<std::vector<float>> m_uvCoords; // 3 floats
  std::vector<uint32_t> m_indices;
  std::vector<material> m_materials;
};

}

#endif
