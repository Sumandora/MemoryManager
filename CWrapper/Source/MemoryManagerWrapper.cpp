#include "MemoryManager/MemoryManager.hpp"
#include "MemoryManager/MemoryManager.h"
#include <cstring>

extern "C" {

void mmgr_get_pointer(const void* memorymanager, void* pointer, uintptr_t address) {
	*static_cast<MemoryManager::Pointer*>(pointer) = static_cast<const MemoryManager::MemoryManager*>(memorymanager)->getPointer(address);
}
const void* mmgr_get_layout(const void* memorymanager) {
	return static_cast<const MemoryManager::MemoryManager*>(memorymanager)->getLayout();
}
void mmgr_update(void* memorymanager) {
	return static_cast<MemoryManager::MemoryManager*>(memorymanager)->update();
}

std::uintptr_t mmgr_allocate(const void* memorymanager, uintptr_t address, size_t size, int protection) {
	return static_cast<const MemoryManager::MemoryManager*>(memorymanager)->allocate(address, size, protection);
}
void mmgr_deallocate(const void* memorymanager, uintptr_t address, size_t size) {
	return static_cast<const MemoryManager::MemoryManager*>(memorymanager)->deallocate(address, size);
}

void mmgr_cleanup_manager(void* memorymanager) {
	static_cast<MemoryManager::MemoryManager*>(memorymanager)->MemoryManager::~MemoryManager();
}

using namespace MemoryManager;

size_t mmgr_sizeof_pointer = sizeof(Pointer);

void mmgr_pointer_read(const void* pointer, void* result, size_t length) {
	return static_cast<const Pointer*>(pointer)->read(result, length);
}
void mmgr_pointer_write(const void* pointer, const void* result, size_t length) {
	return static_cast<const Pointer*>(pointer)->write(result, length);
}
uintptr_t mmgr_pointer_get_address(const void* pointer) {
	return *static_cast<const Pointer*>(pointer);
}

bool mmgr_flags_is_readable(const void* flags) {
	return static_cast<const Flags*>(flags)->isReadable();
}
bool mmgr_flags_is_writeable(const void* flags) {
	return static_cast<const Flags*>(flags)->isWriteable();
}
bool mmgr_flags_is_executable(const void* flags) {
	return static_cast<const Flags*>(flags)->isExecutable();
}
bool mmgr_flags_is_private(const void* flags) {
	return static_cast<const Flags*>(flags)->isPrivate();
}
const char* mmgr_flags_as_string(const void* flags) {
	return strdup(static_cast<const Flags*>(flags)->asString().c_str());
}

const void* mmgr_region_get_flags(const void* region) {
	return &static_cast<const MemoryRegion*>(region)->flags;
}

bool mmgr_region_has_name(const void* region) {
	return static_cast<const MemoryRegion*>(region)->name.has_value();
}
const char* mmgr_region_get_name(const void* region) {
	return strdup(static_cast<const MemoryRegion*>(region)->name.value().c_str());
}

void mmgr_region_get_begin(const void* region, void* pointer) {
	*static_cast<Pointer*>(pointer) = static_cast<const MemoryRegion*>(region)->begin();
}
void mmgr_region_get_end(const void* region, void* pointer) {
	*static_cast<Pointer*>(pointer) = static_cast<const MemoryRegion*>(region)->end();
}
bool mmgr_region_is_inside(const void* region, uintptr_t address) {
	return static_cast<const MemoryRegion*>(region)->isInside(address);
}

const void* mmgr_layout_find_region(const void* layout, uintptr_t address) {
	return static_cast<const MemoryLayout*>(layout)->findRegion(address);
}
size_t mmgr_layout_size(const void* layout) {
	return static_cast<const MemoryLayout*>(layout)->size();
}
const void* mmgr_layout_at(const void* layout, size_t index) {
	// Slow and ugly, but there is really no magic that I can pull here
	auto it = static_cast<const MemoryLayout*>(layout)->begin();
	return &*std::next(it, static_cast<std::iterator_traits<decltype(it)>::difference_type>(index));
}

void mmgr_cleanup_pointer(void* pointer) {
	static_cast<Pointer*>(pointer)->Pointer::~Pointer();
}

}