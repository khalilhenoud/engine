/**
 * @file loader_csv.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef LOADER_CSV_H
#define LOADER_CSV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <loaders/internal/module.h>
#include <library/allocator/allocator.h>
#include <loaders/loader_csv_font_data.h>


LOADERS_API
loader_csv_font_data_t* 
load_csv(const char* path, const allocator_t* allocator);

LOADERS_API
void
free_csv(loader_csv_font_data_t* data, const allocator_t* allocator);


#ifdef __cplusplus
}
#endif

#endif