file(GLOB_RECURSE KACE_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_library(kace ${KACE_SOURCES})

target_include_directories(kace PUBLIC include)
