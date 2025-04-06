/**
 * @file bin_to_scene.c
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <application/process/levels/load_scene.h>
#include <entity/c/scene/scene.h>
#include <library/allocator/allocator.h>
#include <library/containers/cvector.h>
#include <library/filesystem/io.h>
#include <library/string/cstring.h>
#include <library/streams/binary_stream.h>


scene_t*
load_scene(
  const char* dataset, 
  const char* folder, 
  const char* file,
  const allocator_t* allocator)
{
  // TODO: This is temporary, this needs to be generalized in packaged content.
  char fullpath[1024] = {0};
  snprintf(fullpath, 1024, "%s\\%s\\%s.bin", dataset, folder, file);
  
  scene_t *scene;
  binary_stream_t stream;

  binary_stream_def(&stream);
  binary_stream_setup(&stream, allocator);

  {
    size_t read = 0;
    uint8_t buffer[16 * 1024];
    file_handle_t file;
    file = open_file(fullpath, FILE_OPEN_MODE_READ | FILE_OPEN_MODE_BINARY);
    assert((void*)file != NULL);
    do {
      read = read_buffer(
        file,
        buffer, sizeof(uint8_t), 16 * 1024);
      binary_stream_write(&stream, buffer, 16 * 1024);
    } while (read);
    close_file(file);

    scene = scene_create(NULL, allocator);
    scene_deserialize(scene, allocator, &stream);
  }

  binary_stream_cleanup(&stream);
  return scene;
}