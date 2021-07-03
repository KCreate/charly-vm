cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_COMPILER_NAMES clang)
set(CMAKE_CXX_COMPILER_NAMES clang++)
project(charly-vm)

# osx check
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message("-- Detected apple installation")
  add_compile_definitions(APPLE)
  set(APPLE TRUE)
endif()

# public headers
include_directories(include)

# public headers
include_directories(libs)

# main executable
add_subdirectory(src)
add_executable(charly src/main.cpp)
target_link_libraries(charly libcharly)
target_include_directories(charly PRIVATE src)
target_compile_features(charly PRIVATE cxx_std_17)
target_compile_options(charly PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wno-vla-extension
  -Wno-c++20-designator
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-c99-extensions
  -Wno-register
  -ftemplate-backtrace-limit=8
  -march=native
  -flto
)
set_target_properties(charly PROPERTIES LINK_FLAGS -fuse-ld=lld)

# install to global folder
install(TARGETS charly RUNTIME DESTINATION bin)

# unit tests
add_subdirectory(libs/catch2)

file(GLOB tests_SRC
  "test/runtime/core/compiler/diagnostic.cpp"
  "test/runtime/core/compiler/parser.cpp"
  "test/runtime/core/compiler/pass.cpp"
  "test/runtime/core/compiler/semantic.cpp"
  "test/runtime/core/compiler/module.cpp"
  "test/runtime/utils/buffer.cpp"
  "test/runtime/utils/cast.cpp"
  "test/runtime/utils/taggedvalue.cpp"
)

add_executable(tests ${tests_SRC})
target_link_libraries(tests Catch2::Catch2WithMain)
target_link_libraries(tests libcharly)
target_include_directories(tests PRIVATE src)
target_compile_features(tests PRIVATE cxx_std_17)
target_compile_options(tests PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wno-vla-extension
  -Wno-c++20-designator
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-c99-extensions
  -Wno-register
  -ftemplate-backtrace-limit=8
  -march=native
  -flto
)
set_target_properties(tests PROPERTIES LINK_FLAGS -fuse-ld=lld)
