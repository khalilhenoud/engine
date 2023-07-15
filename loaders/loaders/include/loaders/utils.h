/**
 * @file utils.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef LOADERS_UTILS_INCLUDED_H
#define LOADERS_UTILS_INCLUDED_H

#ifdef __cpluscplus
extern "C" {
#endif

#include <loaders/internal/module.h>
#include <library/allocator/allocator.h>


LOADERS_API
const char*
read_file_as_ascii(
  const char* path, 
  const allocator_t* allocator, 
  size_t* file_size);


#ifdef __cpluscplus
}
#endif
#endif