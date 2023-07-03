/**
 * @file png_to_image.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-07-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <application/converters/png_to_image.h>
#include <loaders/loader_png.h>
#include <entity/image.h>
#include <cassert>


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