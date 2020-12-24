cmake_minimum_required(VERSION 3.12)
project(charly-vm)

include_directories(include)

add_subdirectory(src)

add_executable(charly src/main.cpp)
target_link_libraries(charly libcharly)
target_include_directories(charly PRIVATE src)
