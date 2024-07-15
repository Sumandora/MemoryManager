add_subdirectory("../Modules/Linux" "LinuxMemoryManager")

add_executable(MemoryManagerExample "Source/Main.cpp")
target_link_libraries(MemoryManagerExample LinuxMemoryManager)

add_test(NAME TestMemoryManager COMMAND $<TARGET_FILE:MemoryManagerExample>)
