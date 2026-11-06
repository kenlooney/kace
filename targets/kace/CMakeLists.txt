file(GLOB_RECURSE KACE_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "include/*.h")
add_library(kace SHARED ${KACE_SOURCES})
target_include_directories(kace PUBLIC include)
target_link_libraries(kace PRIVATE ${CMAKE_DL_LIBS})
