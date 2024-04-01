include_guard()

project(ExternalMemoryManager)
add_library(ExternalMemoryManager STATIC
        "${PROJECT_SOURCE_DIR}/Source/ExternalMemoryManager.cpp"
)
target_include_directories(ExternalMemoryManager PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(ExternalMemoryManager PRIVATE cxx_std_20)
target_compile_options(ExternalMemoryManager PRIVATE "-Ofast")

include("FetchContent")

FetchContent_Declare(UnException
        GIT_REPOSITORY https://github.com/Sumandora/UnException
        GIT_PROGRESS TRUE
        GIT_TAG 3d64540)
FetchContent_MakeAvailable(UnException)
target_link_libraries(ExternalMemoryManager UnException)

target_link_libraries(ExternalMemoryManager MemoryManager)
