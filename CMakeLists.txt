cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_COMPILER_NAMES clang)
set(CMAKE_CXX_COMPILER_NAMES clang++)
project(charly-vm)

# create build directories
file(MAKE_DIRECTORY cmake-build-debug cmake-build-release)

# osx check
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message("-- Detected apple installation")
  add_compile_definitions(APPLE)
  set(APPLE TRUE)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE OSXSDKPATH)
  include_directories(BEFORE SYSTEM ${OSXSDKPATH})
endif()

# include directories
include_directories(src)
include_directories(include)
include_directories(libs)

# explicitly enable debug symbols in debug builds
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# main executable
add_subdirectory(src)
add_executable(charly src/main.cpp)
target_compile_features(charly PRIVATE cxx_std_17)
target_link_libraries(charly libcharly)
target_compile_options(charly PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wno-vla-extension
  -Wno-c++20-designator
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-gnu-label-as-value
  -Wno-c99-extensions
  -Wno-register
  -Wno-extra-semi
  -ftemplate-backtrace-limit=8
)

# enable link-time-optimization for release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
  message("-- Enabling LTO for target 'charly'")
  target_compile_options(charly PRIVATE -flto)
endif()

# install to global folder
install(TARGETS charly RUNTIME DESTINATION bin)

# unit tests
add_subdirectory(libs/catch2)

file(GLOB tests_SRC
  "test/runtime/core/compiler/diagnostic.cpp"
  "test/runtime/core/compiler/module.cpp"
  "test/runtime/core/compiler/parser.cpp"
  "test/runtime/core/compiler/pass.cpp"
  "test/runtime/core/compiler/semantic.cpp"
  "test/runtime/utils/buffer.cpp"
  "test/runtime/utils/cast.cpp"
  "test/runtime/utils/cast.cpp"
  "test/runtime/utils/random_device.cpp"
  "test/runtime/utils/wait_flag.cpp"
  "test/runtime/value.cpp"
)

add_executable(tests ${tests_SRC})
target_compile_features(tests PRIVATE cxx_std_17)
target_link_libraries(tests Catch2::Catch2WithMain)
target_link_libraries(tests libcharly)
target_compile_definitions(tests PUBLIC CATCH_CONFIG_PREFIX_ALL)
target_compile_options(tests PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wno-vla-extension
  -Wno-c++20-designator
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-gnu-label-as-value
  -Wno-c99-extensions
  -Wno-register
  -Wno-extra-semi
  -ftemplate-backtrace-limit=8
)

# enable link-time-optimization for release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
  message("-- Enabling LTO for target 'tests'")
  target_compile_options(tests PRIVATE -flto)
endif()

if(APPLE)
  set_target_properties(charly PROPERTIES LINK_FLAGS -fuse-ld=ld)
  set_target_properties(tests PROPERTIES LINK_FLAGS -fuse-ld=ld)
else()
  set_target_properties(charly PROPERTIES LINK_FLAGS -fuse-ld=lld)
  set_target_properties(tests PROPERTIES LINK_FLAGS -fuse-ld=lld)
endif()
