# Memory Manager

This is a memory manager that allows memory operations on unreadable/unwritable memory regions under Linux.  
It is also an attempt of making external and internal mod (and game hacking) development uniform and compatible.

## Features

- Provides a uniform interface for internal and external memory management
- Allows reading and writing to restricted memory regions
- Automatically detects memory layout and regions
- Finds memory regions from pointers

## Usage

The memory manager can be used in two modes:

### Local Memory Management

For managing local process memory:

```c++
#define MEMORYMANAGER_DEFINE_PTR_WRAPPER
#include "MemoryManager/LocalMemoryManager.hpp" 

// Create manager
MemoryManager::LocalMemoryManager memoryManager;

// Allocate a memory page
void* ptr = memoryManager.allocate(nullptr, ..., PROT_NONE);

// Make the memory manager aware of new memory sections
memoryManager.update();

// Read the memory on the new page
auto value = memoryManager.read<T>(ptr);
// Write to the memory on the new page
memoryManager.write<T>(ptr, ...);
```

The `MEMORYMANAGER_DEFINE_PTR_WRAPPER` macro can be used to automate void* -> uintptr_t conversions.

### External Memory Management

For managing external process memory:

```c++
#include "MemoryManager/ExternalMemoryManager.hpp"

// Create manager
MemoryManager::ExternalMemoryManager memoryManager(processId);

// Make the memory manager aware of all memory sections
memoryManager.update();

// Read the memory at "ptr"
auto value = memoryManager.read<T>(ptr);
// Write to the memory at "ptr"
memoryManager.write<T>(ptr, ...);
```

## Implementation

The memory manager detects memory regions by parsing `/proc/[pid]/maps`. It maintains an internal layout of the memory regions.  
Reads and writes to protected memory are done through `/proc/[pid]/mem`, which works because of [magic](https://offlinemark.com/2021/05/12/an-obscure-quirk-of-proc/).
