/**
 * @file loader_png.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <loaders/loader_png.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libpng/png.h>
#include <libpng/pngstruct.h>
#include <libpng/pngpriv.h>
#ifdef __cplusplus
}
#endif


loader_png_data_t*
load_png(
  const char* path,
  const allocator_t* allocator)
{
  loader_png_data_t* data = (loader_png_data_t*)allocator->mem_alloc(sizeof(loader_png_data_t));

  png_byte color_type;
  png_byte bit_depth;
  png_byte pixeldepth;
  png_bytep* row_pointers;

  FILE* fp = fopen(path, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png)
    abort();

  png_infop info = png_create_info_struct(png);
  if (!info)
    abort();

  if (setjmp(png_jmpbuf(png)))
    abort();

  png_init_io(png, fp);
  png_read_info(png, info);

  data->width = png_get_image_width(png, info);
  data->height = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);
  pixeldepth = info->pixel_depth;

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt
  png_set_expand(png);

  if (bit_depth == 16)
    png_set_strip_16(png);

  png_read_update_info(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);
  pixeldepth = info->pixel_depth;

  int components = pixeldepth / 8;

  switch (color_type)
  {
  case PNG_COLOR_MASK_COLOR:
  case PNG_COLOR_TYPE_GRAY:
    data->format = (components == 3) ? LOADER_PNG_RGB : LOADER_PNG_L;
    break;
  case PNG_COLOR_TYPE_RGB_ALPHA:
    data->format = LOADER_PNG_RGBA;
    break;
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    data->format = LOADER_PNG_LA;
    break;
  }

  data->buffer = (uint8_t*)allocator->mem_alloc(info->rowbytes * png->height);
  data->total_buffer_size = info->rowbytes * png->height;
  memset(data->buffer, 0, info->rowbytes * png->height);
  row_pointers = (png_bytep*)allocator->mem_alloc(sizeof(png_bytep) * data->height);

  for (int32_t y = 0; y < (int32_t)data->height; y++)
    row_pointers[y] = &data->buffer[(data->height - y - 1) * info->rowbytes];

  png_read_image(png, row_pointers);
  allocator->mem_free(row_pointers);
  fclose(fp);
  return data;
}

void
free_png(
  loader_png_data_t* data, 
  const allocator_t* allocator)
{
  if (data->buffer)
    allocator->mem_free(data->buffer);
  allocator->mem_free(data);
}