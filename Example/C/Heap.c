#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>

#define MEMORYMANAGER_DEFINE_PTR_WRAPPER
#include "MemoryManager/LocalMemoryManager.h"

uintptr_t get_address_from_pointer(const void* region, void(*get_pointer)(const void* region, void* pointer)) {
	void* pointer = malloc(mmgr_sizeof_pointer);
	get_pointer(region, pointer);
	uintptr_t addr = mmgr_pointer_get_address(pointer);
	mmgr_cleanup_pointer(pointer);
	free(pointer);
	return addr;
}

int main()
{
	void* memoryManager = malloc(mmgr_sizeof_localmmgr);
	mmgr_construct_local(memoryManager, MEMORYMANAGER_READ_AND_WRITE);

	int* myInteger = (int*)mmgr_allocate(memoryManager, NULL, sizeof(int), PROT_NONE);
	mmgr_update(memoryManager);

	const void* layout = mmgr_get_layout(memoryManager);
	size_t size = mmgr_layout_size(layout);
	for(size_t i = 0; i < size; i++) {
		const void* region = mmgr_layout_at(layout, i);
		const char* name = "(empty)";
		if(mmgr_region_has_name(region))
			name = mmgr_region_get_name(region);
		uintptr_t begin = get_address_from_pointer(region, mmgr_region_get_begin);
		uintptr_t end = get_address_from_pointer(region, mmgr_region_get_end);
		printf("%s %s %lx %lx\n", name, mmgr_flags_as_string(mmgr_region_get_flags(region)), begin, end);
	}

	printf("Allocated memory at %lx\n", (uintptr_t)myInteger);

	const void* region = mmgr_layout_find_region(layout, (uintptr_t)myInteger + sizeof(int) / 2 /* Prevent us from cheating */);
	assert(region != NULL);
	uintptr_t begin = get_address_from_pointer(region, mmgr_region_get_begin);
	const void* sameRegion = mmgr_layout_find_region(layout, begin);
	assert(region == sameRegion);

	uintptr_t end = get_address_from_pointer(region, mmgr_region_get_end);

	printf("Page region: %lx-%lx\n", begin, end);

	void* myIntegerPtr = malloc(mmgr_sizeof_pointer);
	mmgr_get_pointer(memoryManager, myIntegerPtr, (uintptr_t)myInteger);
	int before;
	mmgr_pointer_read(myIntegerPtr, &before, sizeof(before));
	printf("Before writing: %d\n", before);
	int new = 123;
	mmgr_pointer_write(myIntegerPtr, &new, sizeof(new));
	int after;
	mmgr_pointer_read(myIntegerPtr, &after, sizeof(after));
	printf("After writing: %d\n", after);
	assert(after == new);

	mmgr_cleanup_pointer(myIntegerPtr);
	free(myIntegerPtr);

	mmgr_cleanup_manager(memoryManager);
	free(memoryManager);
	return 0;
}
