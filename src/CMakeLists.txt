file(GLOB charlycore_SRC
  "charly/charly.cpp"
  "charly/core/compiler/ast.cpp"
  "charly/core/compiler/codegenerator.cpp"
  "charly/core/compiler/compiler.cpp"
  "charly/core/compiler/diagnostic.cpp"
  "charly/core/compiler/ir/assembler.cpp"
  "charly/core/compiler/ir/builder.cpp"
  "charly/core/compiler/ir/ir.cpp"
  "charly/core/compiler/lexer.cpp"
  "charly/core/compiler/location.cpp"
  "charly/core/compiler/parser.cpp"
  "charly/core/compiler/passes/class_constructor_check.cpp"
  "charly/core/compiler/passes/constant_folding_pass.cpp"
  "charly/core/compiler/passes/desugar_pass.cpp"
  "charly/core/compiler/passes/duplicates_check.cpp"
  "charly/core/compiler/passes/grammar_validation_check.cpp"
  "charly/core/compiler/passes/repl_prepare_pass.cpp"
  "charly/core/compiler/passes/reserved_identifiers_check.cpp"
  "charly/core/compiler/passes/variable_analyzer_pass.cpp"
  "charly/core/runtime/compiled_module.cpp"
  "charly/core/runtime/gc.cpp"
  "charly/core/runtime/heap.cpp"
  "charly/core/runtime/interpreter.cpp"
  "charly/core/runtime/processor.cpp"
  "charly/core/runtime/runtime.cpp"
  "charly/core/runtime/scheduler.cpp"
  "charly/core/runtime/thread.cpp"
  "charly/core/runtime/worker.cpp"
  "charly/core/runtime/builtins/core.cpp"
  "charly/handle.cpp"
  "charly/utils/argumentparser.cpp"
  "charly/utils/buffer.cpp"
  "charly/utils/allocator.cpp"
  "charly/value.cpp"
  "libs/termcolor/termcolor.cpp"
)

# explicitly enable debug symbols in debug builds
set(CMAKE_CXX_FLAGS_DEBUG "-g")

find_package(Boost REQUIRED COMPONENTS context)

# osx check
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message("-- Detected apple installation")
  add_compile_definitions(APPLE)
  set(APPLE TRUE)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE OSXSDKPATH)
  include_directories(BEFORE SYSTEM ${OSXSDKPATH})
endif()

add_library(libcharly ${charlycore_SRC})
target_compile_features(libcharly PRIVATE cxx_std_17)
if (Boost_FOUND)
  target_include_directories(libcharly PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(libcharly Boost::context)
endif()
target_link_libraries(libcharly stdc++)
target_link_libraries(libcharly m)
target_link_libraries(libcharly readline)
target_link_libraries(libcharly pthread)
target_include_directories(libcharly PRIVATE "./")
target_compile_options(libcharly PRIVATE
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
  -mllvm -align-all-functions=3
)

# enable link-time-optimization for release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
  message("-- Enabling LTO for target 'libcharly'")
  target_compile_options(libcharly PRIVATE -flto)
endif()

set_target_properties(libcharly PROPERTIES PREFIX "")

if(APPLE)
  set_target_properties(libcharly PROPERTIES LINK_FLAGS -fuse-ld=ld)
else()
  set_target_properties(libcharly PROPERTIES LINK_FLAGS -fuse-ld=lld)
endif()