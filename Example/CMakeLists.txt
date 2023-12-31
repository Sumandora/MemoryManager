add_subdirectory("../Modules/Local" "LocalMemoryManager")
# Include C Support
add_subdirectory("../CWrapper" "CWrapper")
add_subdirectory("../Modules/Local/CWrapper" "LocalMemoryManagerCWrapper")

add_executable(ExampleCXX "CXX/Main.cpp")
target_link_libraries(ExampleCXX LocalMemoryManager)


add_executable(ExampleCHeap "C/Heap.c")
target_link_libraries(ExampleCHeap LocalMemoryManagerCWrapper)
target_compile_options(ExampleCHeap PRIVATE "-Wno-int-conversion" "-Wno-pointer-integer-compare")

add_executable(ExampleCStack "C/Stack.c")
target_link_libraries(ExampleCStack LocalMemoryManagerCWrapper)
target_compile_options(ExampleCStack PRIVATE "-Wno-int-conversion" "-Wno-pointer-integer-compare")

add_test(NAME TestCXX COMMAND $<TARGET_FILE:ExampleCXX>)
add_test(NAME TestCHeap COMMAND $<TARGET_FILE:ExampleCHeap>)
add_test(NAME TestCStack COMMAND $<TARGET_FILE:ExampleCStack>)