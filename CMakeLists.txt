cmake_minimum_required(VERSION 3.20)

include_guard()

project(MemoryManager)
add_library(MemoryManager STATIC
	"${PROJECT_SOURCE_DIR}/Source/MemoryManager.cpp"
	"${PROJECT_SOURCE_DIR}/Source/MemoryLayout.cpp"
	"${PROJECT_SOURCE_DIR}/Source/Cache/CacheRegion.cpp"
	"${PROJECT_SOURCE_DIR}/Source/Cache/CacheIterator.cpp"
)

target_include_directories(MemoryManager PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(MemoryManager PRIVATE cxx_std_23)

if(PROJECT_IS_TOP_LEVEL)
	enable_testing()
	add_subdirectory("Example")
endif()

