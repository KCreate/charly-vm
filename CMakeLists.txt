cmake_minimum_required(VERSION 3.12)
project(charly-vm)

# public headers
include_directories(include)

# main executable
add_subdirectory(src)
add_executable(charly src/main.cpp)
target_link_libraries(charly libcharly)
target_include_directories(charly PRIVATE src)
target_compile_features(charly PRIVATE cxx_std_17)

# install to global folder
install(TARGETS charly RUNTIME DESTINATION bin)

# unit tests
find_package(Catch2)
IF(Catch2_FOUND)
  file(GLOB tests_SRC
    "test/runtime/main.cpp"
    "test/runtime/utils/*.cpp"
  )

  add_executable(tests ${tests_SRC})
  target_link_libraries(tests Catch2::Catch2)
  target_include_directories(tests PRIVATE src)
  target_compile_features(tests PRIVATE cxx_std_17)
ELSE()
  message("-- Could not find 'Catch2' library, omitting build target 'tests'")
ENDIF()
