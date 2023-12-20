/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-12-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef RENDER_DATA_UTILS_PROCESS
#define RENDER_DATA_UTILS_PROCESS

#ifdef __cplusplus
extern "C" {
#endif


typedef struct packaged_scene_render_data_t packaged_scene_render_data_t;
typedef struct allocator_t allocator_t;
typedef struct pipeline_t pipeline_t;
typedef struct camera_t camera_t;

// NOTE: call this function before using packaged render data.
void
prep_packaged_render_data(
  const char* data_set,
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator);

void
cleanup_packaged_render_data(
  packaged_scene_render_data_t* render_data, 
  const allocator_t* allocator);

void
render_packaged_scene_data(
  packaged_scene_render_data_t* render_data,
  pipeline_t* pipeline,
  camera_t* camera);

#ifdef __cplusplus
}
#endif

#endif