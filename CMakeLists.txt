cmake_minimum_required(VERSION 3.12)
project(charly-vm)

# public headers
include_directories(include)

# libraries
find_package(Catch2 REQUIRED)

# main executable
add_subdirectory(src)
add_executable(charly src/main.cpp)
target_link_libraries(charly libcharly)
target_include_directories(charly PRIVATE src)

# unit tests
file(GLOB tests_SRC
  "test/runtime/main.cpp"
  "test/runtime/utils/*.cpp"
)

add_executable(tests ${tests_SRC})
target_link_libraries(tests Catch2::Catch2)
target_include_directories(tests PRIVATE src)
