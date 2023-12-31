include_guard()

project(LocalMemoryManagerCWrapper)

add_library(LocalMemoryManagerCWrapper STATIC "${PROJECT_SOURCE_DIR}/Source/LocalMemoryManagerWrapper.cpp")
target_include_directories(LocalMemoryManagerCWrapper PUBLIC "Include")
target_compile_options(LocalMemoryManagerCWrapper PRIVATE "-Ofast")
target_compile_features(LocalMemoryManagerCWrapper PRIVATE cxx_std_20)

add_subdirectory("../../External/CWrapper" "ExternalMemoryManagerCWrapper")
target_link_libraries(LocalMemoryManagerCWrapper PUBLIC LocalMemoryManager ExternalMemoryManagerCWrapper)
