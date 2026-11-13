cmake_minimum_required(VERSION 3.24)
project(kace VERSION 0.30.0)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
include(CTest)

# Expose the project version to CI (read by .github/workflows/release.yml).
file(WRITE ${CMAKE_BINARY_DIR}/kace-version.txt "${PROJECT_VERSION}")

add_subdirectory(targets)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Keep the executable and its shared libraries together in portable archives.
if(UNIX AND NOT APPLE)
  set_target_properties(kasm PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(kasmcore PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(kace PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
install(TARGETS kasm kasmcore kace x86
  RUNTIME DESTINATION .
  LIBRARY DESTINATION .)
install(FILES README.md LICENSE DESTINATION .)

include(CPack)
