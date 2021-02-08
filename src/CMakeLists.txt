file(GLOB charlycore_SRC
  "charly/core/compiler.cpp"
  "charly/core/compiler/ast.cpp"
  "charly/core/compiler/diagnostic.cpp"
  "charly/core/compiler/lexer.cpp"
  "charly/core/compiler/parser.cpp"
  "charly/core/compiler/passes/class_constructor_check.cpp"
  "charly/core/compiler/passes/duplicates_check.cpp"
  "charly/core/compiler/passes/reserved_identifiers_check.cpp"
)


add_library(libcharly ${charlycore_SRC})
target_include_directories(libcharly PRIVATE "./")
target_compile_features(libcharly PRIVATE cxx_std_17)
target_compile_options(libcharly PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-vla-extension -Wno-c++20-designator)
set_target_properties(libcharly PROPERTIES PREFIX "")
