include_guard()

project(ExternalMemoryManager)
add_library(ExternalMemoryManager INTERFACE)
target_include_directories(ExternalMemoryManager INTERFACE "${PROJECT_SOURCE_DIR}/Include")

target_link_libraries(ExternalMemoryManager INTERFACE MemoryManager)
