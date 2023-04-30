/**
 * @file main.cpp
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-01-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <loaders/loader_csv.h>
#include <loaders/loader_ase.h>
#include <loaders/loader_png.h>
#include <library/allocator/allocator.h>
#include <malloc.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>


std::vector<uintptr_t> allocated;

void* allocate(size_t size)
{
  void* block = malloc(size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void* container_allocate(size_t count, size_t elem_size)
{
  void* block = calloc(count, elem_size);
  allocated.push_back(uintptr_t(block));
  return block;
}

void free_block(void* block)
{
  allocated.erase(
    std::remove_if(
      allocated.begin(), 
      allocated.end(), 
      [=](uintptr_t elem) { return (uintptr_t)block == elem; }), 
    allocated.end());
  free(block);
}

void test_ase(const char* path, const allocator_t* allocator)
{
  std::cout << "loading ase: " << path << std::endl;
  loader_ase_data_t* data = load_ase(path, allocator);
  free_ase(data, allocator);

  assert(allocated.size() == 0 && "memory leak detected!");
  std::cout << "loading successful" << std::endl;
}

void test_csv(const char* path, const allocator_t* allocator)
{
  std::cout << "loading csv: " << path << std::endl;
  loader_csv_font_data_t* data = load_csv(path, allocator);
  free_csv(data, allocator);

  assert(allocated.size() == 0 && "memory leak detected!");
  std::cout << "loading successful" << std::endl;
}

void test_png(const char* path, const allocator_t* allocator)
{
  std::cout << "loading png: " << path << std::endl;
  loader_png_data_t* data = load_png(path, allocator);
  free_png(data, allocator);

  assert(allocated.size() == 0 && "memory leak detected!");
  std::cout << "loading successful" << std::endl;
}

int main(int argc, char *argv[])
{
  assert(argc >= 2 && "provide path to ase file!");
  const char* ase_file = argv[1];
  assert(argc >= 3 && "provide path to csv file!");
  const char* csv_file = argv[2];
  assert(argc >= 4 && "provide path to png file!");
  const char* png_file = argv[3];

  allocator_t allocator;
  allocator.mem_alloc = allocate;
  allocator.mem_cont_alloc = container_allocate;
  allocator.mem_free = free_block;
  allocator.mem_alloc_alligned = NULL;
  allocator.mem_realloc = NULL;

  test_ase(ase_file, &allocator);
  test_csv(csv_file, &allocator);
  test_png(png_file, &allocator);

  return 0;
}