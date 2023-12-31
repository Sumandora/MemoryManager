include_guard()

project(ExternalMemoryManager)
add_library(ExternalMemoryManager STATIC
        "${PROJECT_SOURCE_DIR}/Source/ExternalMemoryManager.cpp"
)
target_include_directories(ExternalMemoryManager PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(ExternalMemoryManager PRIVATE cxx_std_20)
target_compile_options(ExternalMemoryManager PRIVATE "-Ofast")

target_link_libraries(ExternalMemoryManager MemoryManager)

# Use this to disable exceptions
#target_compile_definitions(ExternalMemoryManager PUBLIC EXTERNALMEMORYMANAGER_DISABLE_EXCEPTIONS)
