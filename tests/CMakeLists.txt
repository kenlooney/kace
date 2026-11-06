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
