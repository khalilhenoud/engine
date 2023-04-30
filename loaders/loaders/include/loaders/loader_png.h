/**
 * @file loader_png.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef LOADER_PNG_H
#define LOADER_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <loaders/internal/module.h>
#include <library/allocator/allocator.h>
#include <loaders/loader_png_data.h>


LOADERS_API
loader_png_data_t*
load_png(const char* path, const allocator_t* allocator);

LOADERS_API
void
free_png(loader_png_data_t* data, const allocator_t* allocator);


#ifdef __cplusplus
}
#endif

#endif