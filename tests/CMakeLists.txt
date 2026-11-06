# Executables
add_executable(kasm_source_load_test kasm_source_load_test.cpp)
target_link_libraries(kasm_source_load_test PRIVATE kace)

# Tests
add_test(
  NAME kasm_source_load_test
  COMMAND kasm_source_load_test
          "${CMAKE_SOURCE_DIR}/examples/kasm_source_load.asm"
)