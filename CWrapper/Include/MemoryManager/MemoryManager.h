#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t mmgr_sizeof_pointer;

void mmgr_pointer_read(const void* pointer, void* result, size_t length);
void mmgr_pointer_write(const void* pointer, const void* result, size_t length);
uintptr_t mmgr_pointer_get_address(const void* pointer);

bool mmgr_flags_is_readable(const void* flags);
bool mmgr_flags_is_writeable(const void* flags);
bool mmgr_flags_is_executable(const void* flags);
bool mmgr_flags_is_private(const void* flags);
const char* mmgr_flags_as_string(const void* flags); // Has to be free'd

const void* mmgr_region_get_flags(const void* region);

bool mmgr_region_has_name(const void* region);
const char* mmgr_region_get_name(const void* region); // NULL when hasName == false; also has to be free'd

void mmgr_region_get_begin(const void* region, void* pointer);
void mmgr_region_get_end(const void* region, void* pointer);
bool mmgr_region_is_inside(const void* region, uintptr_t address);

const void* mmgr_layout_find_region(const void* layout, uintptr_t address);
size_t mmgr_layout_size(const void* layout);
const void* mmgr_layout_at(const void* layout, size_t index);

void mmgr_get_pointer(const void* memorymanager, void* pointer, uintptr_t address);
const void* mmgr_get_layout(const void* memorymanager);
void mmgr_update(void* memorymanager);

void* mmgr_allocate(const void* memorymanager, uintptr_t address, size_t size, int protection);
void mmgr_deallocate(const void* memorymanager, uintptr_t address, size_t size);

void mmgr_cleanup_manager(void* memorymanager);
void mmgr_cleanup_pointer(void* pointer);

#ifdef __cplusplus
}
#endif

#endif
