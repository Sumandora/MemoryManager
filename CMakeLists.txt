cmake_minimum_required(VERSION 3.20)

include_guard()

project(MemoryManager)
add_library(MemoryManager STATIC
	"${PROJECT_SOURCE_DIR}/Source/MemoryManager.cpp"
	"${PROJECT_SOURCE_DIR}/Source/LocalMemoryManager.cpp"
	"${PROJECT_SOURCE_DIR}/Source/ExternalMemoryManager.cpp"
)
target_include_directories(MemoryManager PUBLIC "${PROJECT_SOURCE_DIR}/Include")
target_compile_features(MemoryManager PRIVATE cxx_std_20)
target_compile_options(MemoryManager PRIVATE "-Ofast")

if(PROJECT_IS_TOP_LEVEL)
	add_executable(MemoryManagerExample "${PROJECT_SOURCE_DIR}/Example/Main.cpp")
	target_compile_features(MemoryManagerExample PRIVATE cxx_std_20)
	target_link_libraries(MemoryManagerExample PRIVATE MemoryManager)

	enable_testing()
	add_test(NAME MemoryManagerExample COMMAND $<TARGET_FILE:MemoryManagerExample>)
endif()

