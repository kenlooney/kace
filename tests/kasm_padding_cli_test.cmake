set(source "${TEST_DIR}/padding_test.asm")
set(output "${TEST_DIR}/padding_test.bin")

foreach(expression IN ITEMS "10 - ($ - $$)" "10 - (currentposition - sectionstart)")
  file(WRITE "${source}" "nop\nfill ${expression} db 0\ndb 0x55\n")
  execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                  RESULT_VARIABLE result ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Padding assembly failed: ${error}")
  endif()
  file(READ "${output}" bytes HEX)
  if(NOT bytes STREQUAL "9000000000000000000055")
    message(FATAL_ERROR "Unexpected padding output: ${bytes}")
  endif()
endforeach()

file(WRITE "${source}" "nop\nfill 10 - (currentposition - sectionstart) byte 0\n")
execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                RESULT_VARIABLE result ERROR_VARIABLE error)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Named position and byte aliases failed: ${error}")
endif()
file(READ "${output}" bytes HEX)
if(NOT bytes STREQUAL "90000000000000000000")
  message(FATAL_ERROR "Unexpected named padding output: ${bytes}")
endif()

foreach(repetition IN ITEMS times fill repeat dup)
  foreach(directive IN ITEMS db byte definebyte)
    file(WRITE "${source}" "${repetition} 3 ${directive} 0x2a\n")
    execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                    RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "Alias assembly failed for ${repetition} ${directive}: ${error}")
    endif()
    file(READ "${output}" bytes HEX)
    if(NOT bytes STREQUAL "2a2a2a")
      message(FATAL_ERROR "Unexpected alias output for ${repetition} ${directive}: ${bytes}")
    endif()
  endforeach()
endforeach()

foreach(directive IN ITEMS dw word defineword dd dq)
  file(WRITE "${source}" "fill 2 ${directive} 0\n")
  execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                  RESULT_VARIABLE result ERROR_VARIABLE error)
  if(result EQUAL 0 OR NOT error MATCHES "Only byte repetition is supported")
    message(FATAL_ERROR "Expected unsupported directive error for ${directive}: ${error}")
  endif()
endforeach()

file(WRITE "${source}" "nop\ntimes 0 - ($ - $$) db 0\n")
execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                RESULT_VARIABLE result ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "negative")
  message(FATAL_ERROR "Expected an error for padding past the target: ${error}")
endif()

file(WRITE "${source}" "times 1000001 db 0\n")
execute_process(COMMAND "${KASM}" -i "${source}" -o "${output}"
                RESULT_VARIABLE result ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT error MATCHES "limit")
  message(FATAL_ERROR "Expected a repeat count limit error: ${error}")
endif()
