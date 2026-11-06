file(GLOB_RECURSE KASM_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_executable(kasm ${KASM_SOURCES})
target_link_libraries(kasm kasmcore)
target_compile_definitions(kasm PRIVATE
  KACE_PLUGIN_PREFIX="${CMAKE_SHARED_LIBRARY_PREFIX}"
  KACE_PLUGIN_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
