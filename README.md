# Memory Manager

This is a memory manager that allows memory operations on unreadable/unwritable memory regions under Linux.  
It is also an attempt of making external and internal mod (and game hacking) development uniform and compatible.

## Features

- Provides a uniform interface for internal and external memory management
- Allows reading and writing to restricted memory regions
- Automatically detects memory layout and regions
- Finds memory regions from pointers

## Usage

This repository ships with a Linux default implementation. Once added the memory manager can be used in two modes:

### Local Memory Management

For managing local process memory:

```c++
#include "MemoryManager/LinuxMemoryManager.hpp" 

// Create manager
MemoryManager::LinuxMemoryManager<false /*Force Read*/, true /*Force Write*/, true /*Local*/> memory_manager;

// Allocate a memory page
std::uintptr_t ptr = memory_manager.allocate(memory_manager.get_page_granularity(), PROT_NONE);

// Make the memory manager aware of new memory sections
memory_manager.update();

// Read the memory on the new page
int value;
memory_manager.read(ptr, &value, sizeof(int));
// Write to the memory on the new page
memory_manager.write(ptr, &value, sizeof(int));
```

### External Memory Management

For managing external process memory:

```c++
#include "MemoryManager/LinuxMemoryManager.hpp"

// Create manager
MemoryManager::LinuxMemoryManager<true /*Read*/, true /*Write*/> memory_manager(processId);

// Make the memory manager aware of all memory sections
memory_manager.update();

// Read the memory at "ptr"
int value;
memory_manager.read(ptr, &value, sizeof(int));
// Write to the memory at "ptr"
memory_manager.write(ptr, &value, sizeof(int));
```

## Implementation

The memory manager detects memory regions by parsing `/proc/[pid]/maps`. It maintains an internal layout of the memory regions.  
Force reads and force writes to protected memory are done through `/proc/[pid]/mem`, which works because of [magic](https://offlinemark.com/2021/05/12/an-obscure-quirk-of-proc/).
