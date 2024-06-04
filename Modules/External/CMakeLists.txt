include_guard()

project(ExternalMemoryManager)
add_library(ExternalMemoryManager INTERFACE)
target_include_directories(ExternalMemoryManager INTERFACE "${PROJECT_SOURCE_DIR}/Include")

include("FetchContent")

FetchContent_Declare(UnException
        GIT_REPOSITORY https://github.com/Sumandora/UnException
        GIT_PROGRESS TRUE
        GIT_TAG 1.0)
FetchContent_MakeAvailable(UnException)
target_link_libraries(ExternalMemoryManager INTERFACE UnException MemoryManager)
