file(GLOB_RECURSE X86_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_library(x86 SHARED ${X86_SOURCES})
target_link_libraries(x86 kace kasmcore)
target_include_directories(x86 PUBLIC include)
