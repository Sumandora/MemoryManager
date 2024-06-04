cmake_minimum_required(VERSION 3.20)

include_guard()

project(MemoryManager)
add_library(MemoryManager INTERFACE)

target_include_directories(MemoryManager INTERFACE "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(MemoryManager INTERFACE cxx_std_23)

if (PROJECT_IS_TOP_LEVEL)
    enable_testing()
    add_subdirectory("Example")
endif ()

