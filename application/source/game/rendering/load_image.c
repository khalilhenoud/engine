/**
 * @file png_to_image.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <application/game/rendering/load_image.h>
#include <library/allocator/allocator.h>
#include <library/string/cstring.h>
#include <library/string/fixed_string.h>
#include <loaders/loader_png.h>
#include <entity/c/runtime/texture.h>
#include <entity/c/runtime/texture_utils.h>


void
load_image_buffer(
  const char* data_set, 
  image_t* image, 
  const allocator_t* allocator)
{
  assert(data_set && image && allocator);

  {
    uint8_t extension[8];
    get_image_extension(image, extension);
    assert(!strcmp(extension, "png") && "we only support loading of pngs!");

    {
      fixed_str_t path;
      memset(path.data, 0, sizeof(path.data));
      sprintf(path.data, "%s%s", data_set, image->texture.path.str);

      {
        loader_png_data_t* data = load_png(path.data, allocator);
        assert(data);

        allocate_runtime_buffer(image, data->total_buffer_size, allocator);
        memcpy(image->buffer, data->buffer, data->total_buffer_size);
        
        // the types are compatible.
        image->format = (image_format_t)data->format;
        image->width = data->width;
        image->height = data->height;
        
        free_png(data, allocator);
      }
    }
  }
}