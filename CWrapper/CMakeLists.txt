include_guard()

project(MemoryManagerCWrapper)

add_library(MemoryManagerCWrapper STATIC "${PROJECT_SOURCE_DIR}/Source/MemoryManagerWrapper.cpp")
target_include_directories(MemoryManagerCWrapper PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_options(MemoryManagerCWrapper PRIVATE "-Ofast")
target_compile_features(MemoryManagerCWrapper PRIVATE cxx_std_20)

target_link_libraries(MemoryManagerCWrapper PUBLIC MemoryManager)
