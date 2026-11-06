file(GLOB_RECURSE KACE_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_library(kasmcore SHARED ${KACE_SOURCES})
target_link_libraries(kasmcore kace)
target_include_directories(kasmcore PUBLIC include)
