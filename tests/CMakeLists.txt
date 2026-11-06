# Executables
add_executable(kasm_source_load_test kasm_source_load_test.cpp)
target_link_libraries(kasm_source_load_test PRIVATE kace)

add_executable(kasm_diagnostic_test kasm_diagnostic_test.cpp)
target_link_libraries(kasm_diagnostic_test PRIVATE kasmcore)

# Tests
add_test(
  NAME kasm_source_load_test
  COMMAND $<TARGET_FILE:kasm_source_load_test>
          "${CMAKE_SOURCE_DIR}/examples/kasm_source_load.asm"
)
add_test(
  NAME kasm_diagnostic_test
  COMMAND $<TARGET_FILE:kasm_diagnostic_test>
          "${CMAKE_SOURCE_DIR}/examples/kasm_source_load.asm"
)