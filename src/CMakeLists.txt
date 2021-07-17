file(GLOB charlycore_SRC
  "charly/core/compiler.cpp"
  "charly/core/compiler/ast.cpp"
  "charly/core/compiler/codegenerator.cpp"
  "charly/core/compiler/diagnostic.cpp"
  "charly/core/compiler/ir/assembler.cpp"
  "charly/core/compiler/ir/builder.cpp"
  "charly/core/compiler/ir/ir.cpp"
  "charly/core/compiler/lexer.cpp"
  "charly/core/compiler/localvars.cpp"
  "charly/core/compiler/location.cpp"
  "charly/core/compiler/parser.cpp"
  "charly/core/compiler/passes/class_constructor_check.cpp"
  "charly/core/compiler/passes/constant_folding_pass.cpp"
  "charly/core/compiler/passes/desugar_pass.cpp"
  "charly/core/compiler/passes/duplicates_check.cpp"
  "charly/core/compiler/passes/grammar_validation_check.cpp"
  "charly/core/compiler/passes/local_allocator_pass.cpp"
  "charly/core/compiler/passes/repl_prepare_pass.cpp"
  "charly/core/compiler/passes/reserved_identifiers_check.cpp"
  "charly/core/runtime/allocator.cpp"
  "charly/core/runtime/compiled_module.cpp"
  "charly/core/runtime/compiled_module.cpp"
  "charly/core/runtime/gc.cpp"
  "charly/core/runtime/scheduler.cpp"
  "charly/core/runtime/vm_frame.cpp"
  "charly/taggedvalue.cpp"
  "charly/value.cpp"
  "charly/utils/argumentparser.cpp"
  "charly/utils/buffer.cpp"
)

find_package(Boost REQUIRED COMPONENTS context)

add_library(libcharly ${charlycore_SRC})
if (Boost_FOUND)
  target_include_directories(libcharly PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(libcharly Boost::context)
endif(Boost_FOUND)
target_link_libraries(libcharly stdc++)
target_link_libraries(libcharly m)
target_link_libraries(libcharly readline)
target_link_libraries(libcharly pthread)
target_include_directories(libcharly PRIVATE "./")
target_compile_features(libcharly PRIVATE cxx_std_17)
target_compile_options(libcharly PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wno-vla-extension
  -Wno-c++20-designator
  -Wno-gnu-zero-variadic-macro-arguments
  -Wno-c99-extensions
  -Wno-register
  -Wno-extra-semi
  -ftemplate-backtrace-limit=8
  -march=native
  -flto
)
set_target_properties(libcharly PROPERTIES LINK_FLAGS -fuse-ld=lld)
set_target_properties(libcharly PROPERTIES PREFIX "")
