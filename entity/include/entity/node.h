/**
 * @file node.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <entity/mesh.h>


namespace entity {

struct
node {

  std::vector<std::string> 
  get_textures_paths()
  {
    std::vector<std::string> paths;
    std::vector<std::string> tmp;

    for (uint32_t i = 0; i < m_meshes.size(); ++i) {
      tmp = m_meshes[i]->get_textures_paths();
      paths.insert(paths.end(), tmp.begin(), tmp.end());
    }

    for (uint32_t i = 0; i < m_children.size(); ++i) {
      tmp = m_children[i]->get_textures_paths();
      paths.insert(paths.end(), tmp.begin(), tmp.end());
    }

    return paths;
  }

  std::vector<mesh*> 
  get_meshes()
  {
    std::vector<mesh*> meshes;
    std::vector<mesh*> tmp;

    for (uint32_t i = 0; i < m_meshes.size(); ++i)
      meshes.push_back(m_meshes[i].get());

    for (uint32_t i = 0; i < m_children.size(); ++i) {
      tmp = m_children[i]->get_meshes();
      meshes.insert(meshes.end(), tmp.begin(), tmp.end());
    }

    return meshes;
  }

  std::vector<node*>
  get_nodes()
  {
    std::vector<node*> nodes;
    std::vector<node*> tmp;

    for (uint32_t i = 0; i < m_children.size(); ++i)
      nodes.push_back(m_children[i].get());

    for (uint32_t i = 0; i < m_children.size(); ++i) {
      tmp = m_children[i]->get_nodes();
      nodes.insert(nodes.end(), tmp.begin(), tmp.end());
    }

    return nodes;
  }

  //////////////////////////////////////////////////////////////////////////////
  std::string m_name;
  std::vector<std::shared_ptr<mesh>> m_meshes;
  std::vector<std::shared_ptr<node>> m_children;
};

}

#endif