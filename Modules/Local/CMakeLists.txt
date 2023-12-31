include_guard()

project(LocalMemoryManager)
add_library(LocalMemoryManager STATIC
        "${PROJECT_SOURCE_DIR}/Source/LocalMemoryManager.cpp"
)
target_include_directories(LocalMemoryManager PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(LocalMemoryManager PRIVATE cxx_std_20)
target_compile_options(LocalMemoryManager PRIVATE "-Ofast")

add_subdirectory("../External" "ExternalMemoryManager")
target_link_libraries(LocalMemoryManager ExternalMemoryManager ExternalMemoryManagerCWrapper)

# Use this to disable exceptions
#target_compile_definitions(LocalMemoryManager PUBLIC LOCALMEMORYMANAGER_DISABLE_EXCEPTIONS)
