include_guard()

project(LinuxMemoryManager)
add_library(LinuxMemoryManager INTERFACE)
target_include_directories(LinuxMemoryManager INTERFACE "${PROJECT_SOURCE_DIR}/Include")

target_link_libraries(LinuxMemoryManager INTERFACE MemoryManager)
