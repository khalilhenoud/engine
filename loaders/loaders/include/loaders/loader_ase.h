/**
 * @file loader_ase.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef LOADER_ASE_H
#define LOADER_ASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <library/allocator/allocator.h>
#include <loaders/internal/module.h>
#include <loaders/loader_ase_mesh_data.h>


LOADERS_API
loader_ase_data_t* 
load_ase(const char* path, const allocator_t *allocator);

LOADERS_API
void
free_ase(loader_ase_data_t* data, const allocator_t *allocator);


#ifdef __cplusplus
}
#endif

#endif