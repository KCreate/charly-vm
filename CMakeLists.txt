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

if(APPLE)
  include_directories(SYSTEM /usr/local/include)
else()
  include_directories(SYSTEM /usr/include)
endif()

# public headers
include_directories(include)

# public headers
include_directories(libs)

# main executable
add_subdirectory(src)
add_executable(charly src/main.cpp)
target_link_libraries(charly libcharly)
target_link_libraries(charly readline)
target_include_directories(charly PRIVATE src)
target_compile_features(charly PRIVATE cxx_std_17)
target_compile_options(charly PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-vla-extension -Wno-c++20-designator)

# install to global folder
install(TARGETS charly RUNTIME DESTINATION bin)

# unit tests
find_package(Catch2)
if(Catch2_FOUND)
  message("-- Detected 'Catch2' library, enabling target 'tests'")

  file(GLOB tests_SRC
    "test/runtime/main.cpp"
    "test/runtime/utils/*.cpp"
    "test/runtime/core/compiler/*.cpp"
  )

  add_executable(tests ${tests_SRC})
  target_link_libraries(tests Catch2::Catch2)
  target_link_libraries(tests libcharly)
  target_include_directories(tests PRIVATE src)
  target_include_directories(tests PRIVATE SYSTEM /usr/include)
  target_compile_features(tests PRIVATE cxx_std_17)
  target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-vla-extension -Wno-c++20-designator)
else()
  message("-- Could not find 'Catch2' library, omitting build target 'tests'")
endif()
