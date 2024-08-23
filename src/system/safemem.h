#ifndef __SAFEMEM__
#define __SAFEMEM__
#include <Arduino.h>

#define MAX_BLOCKS 64
#define MAX_MEMORY 16384 // 16kB

struct memblock {
  bool active = false;
  size_t size = 0;
  uint8_t msg = 0;
  void* ptr = nullptr;

  void _free();
  void _alloc(size_t size);

  memblock();
  memblock(size_t size);
  memblock(size_t size, uint8_t msg);
};


void* my_safe_alloc(size_t size);
void* my_safe_alloc(size_t size, uint8_t msg);
void my_safe_free(void*);

size_t my_safe_max_mem();
size_t my_safe_max_blocks();
size_t my_safe_cur_mem();
size_t my_safe_cur_blocks();
size_t my_safe_fre_mem();
size_t my_safe_fre_blocks();
void my_safe_free_all();
memblock* my_safe_get_blocks();

char* my_safe_strdup(const char* str);

#endif