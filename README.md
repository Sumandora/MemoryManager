# Memory Manager

This is a memory manager that allows memory operations on unreadable/unwritable memory regions under Linux.

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

MemoryManager::LocalMemoryManager memoryManager;

void* ptr = mmap(nullptr, ..., PROT_NONE, ..., 0, 0); 

memoryManager.update();

auto value = memoryManager.getPointer(ptr).read<T>();
```

The `MEMORYMANAGER_DEFINE_PTR_WRAPPER` macro can be used to simplify pointer conversion.

### External Memory Management

For managing external process memory:

```c++
#include "MemoryManager/ExternalMemoryManager.hpp"

MemoryManager::ExternalMemoryManager memoryManager(processId);
memoryManager.update();

auto ptr = memoryManager.getPointer(address);
T value = ptr.read<T>();
```

## Implementation

The memory manager detects memory regions by parsing `/proc/[pid]/maps`. It maintains an internal layout of the memory regions.  
Reads and writes to protected memory are done through `/proc/[pid]/mem`, which works because of [magic](https://offlinemark.com/2021/05/12/an-obscure-quirk-of-proc/).
