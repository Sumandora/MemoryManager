include_guard()

project(LocalMemoryManager)
add_library(LocalMemoryManager INTERFACE)
target_include_directories(LocalMemoryManager INTERFACE "${PROJECT_SOURCE_DIR}/Include")

add_subdirectory("../External" "ExternalMemoryManager")
target_link_libraries(LocalMemoryManager INTERFACE ExternalMemoryManager)
