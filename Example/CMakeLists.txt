add_subdirectory("../Modules/Local" "LocalMemoryManager")

add_executable(MemoryManagerExample "Source/Main.cpp")
target_link_libraries(MemoryManagerExample LocalMemoryManager)

add_test(NAME TestMemoryManager COMMAND $<TARGET_FILE:MemoryManagerExample>)
