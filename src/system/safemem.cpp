#include "safemem.h"


memblock _blocks[MAX_BLOCKS];
int _blocks_count = 0;
int _memory_count = 0;

void memblock::_free() {
  if (this->ptr == nullptr) return;
  if (this->active == false) return;
  free(this->ptr);
  _blocks_count--;
  _memory_count -= this->size;
  this->active = false;
  this->ptr = nullptr;
  this->msg = 0;
  this->size = 0;
}
void memblock::_alloc(size_t size) {
  this->active = true;
  this->size = size;
  this->msg = 0;
  this->ptr = malloc(size);
  memset(this->ptr, 0, size);
  _blocks_count++;
  _memory_count += size;
}
memblock::memblock() {
  this->active = false;
  this->size = 0;
  this->msg = 0;
  this->ptr = nullptr;
}
memblock::memblock(size_t size) {
  this->_free();
  this->_alloc(size);
}
memblock::memblock(size_t size, uint8_t msg) {
  this->_free();
  this->_alloc(size);
  this->msg = msg;
}

int _first_block() {
  for (int i = 0; i < MAX_BLOCKS; i++) {
    if (_blocks[i].active == false) return i;
  }
  return -1;
}
int _find_block(void* ptr) {
  for (int i = 0; i < MAX_BLOCKS; i++) {
    if (_blocks[i].ptr == ptr) return i;
  }
  return -1;
}

void* safe_alloc(size_t size) { return safe_alloc(size, 0); }
void* safe_alloc(size_t size, uint8_t msg) {
  if (_blocks_count >= MAX_BLOCKS) {
    panic();
    return nullptr;
  }
  if (_memory_count + size >= MAX_MEMORY) {
    panic();
    return nullptr;
  }

  int i = _first_block();
  if (i == -1) return nullptr;

  _blocks[i] = memblock(size, msg);
  memset(_blocks[i].ptr, 0, _blocks[i].size);
  return _blocks[i].ptr;
}
void safe_free(void* ptr) {
  int i = _find_block(ptr);
  if (i == -1) return;
  _blocks[i]._free();
}

size_t safe_max_mem() { return MAX_MEMORY; }
size_t safe_max_blocks() { return MAX_BLOCKS; }
size_t safe_cur_mem() { return _memory_count; }
size_t safe_cur_blocks() { return _blocks_count; }
size_t safe_fre_mem() { return MAX_MEMORY - _memory_count; }
size_t safe_fre_blocks() { return MAX_BLOCKS - _blocks_count; }

void safe_free_all() {
  for (int i = 0; i < MAX_BLOCKS; i++) { 
    _blocks[i]._free();
  }
}
memblock* safe_get_blocks() { return &_blocks[0]; }

char* safe_strdup(const char* str) {
  size_t len = strlen(str);
  char* ptr = (char*)safe_alloc(len + 1);
  memcpy(ptr, str, len);
  ptr[len] = 0;
  return ptr;
}
