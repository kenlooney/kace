# Executables
add_executable(kasm_source_load_test kasm_source_load_test.cpp)
target_link_libraries(kasm_source_load_test PRIVATE kace)

add_executable(kasm_diagnostic_test kasm_diagnostic_test.cpp)
target_link_libraries(kasm_diagnostic_test PRIVATE kasmcore)

add_executable(display_tokens_test display_tokens_test.cpp)
target_link_libraries(display_tokens_test PRIVATE kasmcore)

add_executable(integer_parsing_test integer_parsing_test.cpp)
target_link_libraries(integer_parsing_test PRIVATE kace)

add_executable(kasm_parser_expression_test kasm_parser_expression_test.cpp)
target_link_libraries(kasm_parser_expression_test PRIVATE kasmcore)

add_executable(multi_expressions_test multi_expressions_test.cpp)
target_link_libraries(multi_expressions_test PRIVATE kasmcore)

add_executable(kasm_collect_bytes_test kasm_collect_bytes_test.cpp)
target_link_libraries(kasm_collect_bytes_test PRIVATE kasmcore)

add_executable(x86_plugin_test x86_plugin_test.cpp)
target_link_libraries(x86_plugin_test PRIVATE x86)

add_executable(cpu_loader_test cpu_loader_test.cpp)
target_link_libraries(cpu_loader_test PRIVATE kace)

add_executable(kasm_keyword_alias_test kasm_keyword_alias_test.cpp)
target_link_libraries(kasm_keyword_alias_test PRIVATE kasmcore)
add_executable(symbol_table_test symbol_table_test.cpp)
target_link_libraries(symbol_table_test PRIVATE kace)
# Tests
add_test(NAME symbol_table_test COMMAND $<TARGET_FILE:symbol_table_test>)
add_test(NAME x86_plugin_test COMMAND $<TARGET_FILE:x86_plugin_test>)
add_test(NAME kasm_keyword_alias_test COMMAND $<TARGET_FILE:kasm_keyword_alias_test>)
add_test(NAME cpu_loader_test COMMAND $<TARGET_FILE:cpu_loader_test>
  $<TARGET_FILE:x86> $<TARGET_FILE:kace>)
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
add_test(
  NAME display_tokens_test
  COMMAND $<TARGET_FILE:display_tokens_test>
          "${CMAKE_SOURCE_DIR}/examples/kasm_source_load.asm"
)
add_test(
  NAME display_tokens_test_tokens
  COMMAND $<TARGET_FILE:display_tokens_test>
          "${CMAKE_SOURCE_DIR}/examples/tokens.asm"
)
add_test(
  NAME integer_parsing_test
  COMMAND $<TARGET_FILE:integer_parsing_test>
)
add_test(
  NAME kasm_parser_expression_test
  COMMAND $<TARGET_FILE:kasm_parser_expression_test>
          "${CMAKE_SOURCE_DIR}/examples/expression.asm"
)
add_test(
  NAME multi_expressions_test
  COMMAND $<TARGET_FILE:multi_expressions_test>
          "${CMAKE_SOURCE_DIR}/examples/multiexpressions.asm"
)
add_test(
  NAME kasm_collect_bytes_test
  COMMAND $<TARGET_FILE:kasm_collect_bytes_test>
          "${CMAKE_SOURCE_DIR}/examples/multiexpressions.asm"
)
add_test(
  NAME kasm_cpu_cli_test
  COMMAND ${CMAKE_COMMAND}
          -DKASM=$<TARGET_FILE:kasm>
          -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/kasm_cpu_cli_test.cmake
)
add_test(
  NAME kasm_short_jump_cli_test
  COMMAND ${CMAKE_COMMAND}
          -DKASM=$<TARGET_FILE:kasm>
          -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/kasm_short_jump_cli_test.cmake
)
add_test(
  NAME kasm_disassemble_cli_test
  COMMAND ${CMAKE_COMMAND}
          -DKASM=$<TARGET_FILE:kasm>
          -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/kasm_disassemble_cli_test.cmake
)
