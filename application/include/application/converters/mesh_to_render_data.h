/**
 * @file mesh_to_render_data.h
 * @author khalilhenoud@gmail.com
 * @brief acts as glue between different parts of the engine.
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef MESH_TO_RENDER_DATA
#define MESH_TO_RENDER_DATA

#include <renderer/renderer_opengl_data.h>


struct mesh_t;

mesh_render_data_t
load_mesh_renderer_data(mesh_t* mesh, color_t color);

#endif