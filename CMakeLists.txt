cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_COMPILER_NAMES clang)
set(CMAKE_CXX_COMPILER_NAMES clang++)
project(charly-vm)

# create build directories
file(MAKE_DIRECTORY cmake-build-debug cmake-build-release)

# osx check
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message("-- Detected apple installation")
    add_compile_definitions(APPLE)
    set(APPLE TRUE)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE OSXSDKPATH)
    include_directories(BEFORE SYSTEM ${OSXSDKPATH})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include")
endif ()

# --- Detect Arch Linux ---
set(IS_ARCH FALSE)
if(UNIX AND EXISTS "/etc/os-release")
    file(READ "/etc/os-release" OS_RELEASE)
    if(OS_RELEASE MATCHES "ID=arch" OR OS_RELEASE MATCHES "ID_LIKE=arch")
        set(IS_ARCH TRUE)
    endif()
endif()

# --- Detect WSL (Windows Subsystem for Linux) ---
set(IS_WSL FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/proc/version")
    file(READ "/proc/version" PROC_VERSION)
    if(PROC_VERSION MATCHES "Microsoft" OR PROC_VERSION MATCHES "WSL")
        set(IS_WSL TRUE)
    endif()
endif()

# --- Only build tests if NOT Arch and NOT WSL ---
if(NOT IS_ARCH AND NOT IS_WSL)
    option(BUILD_TESTS "Build tests" ON)
else()
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
endif()

add_compile_options(-Wno-nontrivial-memaccess -Wno-missing-field-initializers -Wno-missing-designated-field-initializers)

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
        -Wno-shadow
        -ftemplate-backtrace-limit=8
        -mllvm -align-all-functions=3
        )

# enable link-time-optimization for release builds
if (CMAKE_BUILD_TYPE MATCHES Release)
    message("-- Enabling LTO for target 'charly'")
    target_compile_options(charly PRIVATE -flto -O3)
endif ()

# install to global folder
install(TARGETS charly RUNTIME DESTINATION bin)

if (BUILD_TESTS)
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
          -Wno-shadow
          -ftemplate-backtrace-limit=8
          -mllvm -align-all-functions=3
          )

# enable link-time-optimization for release builds
  if (CMAKE_BUILD_TYPE MATCHES Release)
      message("-- Enabling LTO for target 'tests'")
      target_compile_options(tests PRIVATE -flto -O3)
  endif ()

  if (APPLE)
      set_target_properties(tests PROPERTIES LINK_FLAGS -fuse-ld=ld)
  else ()
      set_target_properties(tests PROPERTIES LINK_FLAGS -fuse-ld=lld)
  endif ()
endif()

if (APPLE)
    set_target_properties(charly PROPERTIES LINK_FLAGS -fuse-ld=ld)
else ()
    set_target_properties(charly PROPERTIES LINK_FLAGS -fuse-ld=lld)
endif ()
