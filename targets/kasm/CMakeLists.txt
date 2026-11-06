file(GLOB_RECURSE KASM_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_executable(kasm ${KASM_SOURCES})
target_link_libraries(kasm kace)
