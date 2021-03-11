/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include <readline/history.h>
#include <readline/readline.h>

#include "charly/utils/buffer.h"

#include "charly/core/compiler.h"
#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/ir/builder.h"

using namespace charly;
using namespace charly::core::compiler;
using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;

int run_repl() {
  bool print_location = false;

  char* buf = nullptr;
  while ((buf = readline("> "))) {
    if (strlen(buf) > 0) {
      add_history(buf);
    }
    std::string line(buf);
    free(buf);

    if (line[0] == '.') {
      if (line.compare(".help") == 0) {
        std::cout << ".exit     Exit the REPL" << '\n';
        std::cout << ".help     List of meta commands" << '\n';
        std::cout << ".ast_loc  Display source locations in AST dump" << '\n';
        continue;
      }

      if (line.compare(".exit") == 0)
        break;

      if (line.compare(".ast_loc") == 0) {
        print_location = !print_location;
        continue;
      }
    }

    utils::Buffer source_buffer(line);
    auto unit = Compiler::compile(CompilationUnit::Type::ReplInput, "stdin", source_buffer);
    auto program = unit->ast;

    unit->console.dump_all(std::cerr);

    if (unit->console.has_errors()) {
      std::cerr << std::endl;
      continue;
    }

    assert(program.get());

    // check if program has nodes
    if (ref<Block> body = cast<Block>(program)) {
      if (body->statements.size() == 0)
        continue;
    }

    // dump compiler steps
    program->dump(std::cout, print_location);
    unit->ir_module->dump(std::cout);
  }

  return 0;
}

int run_file(int, char** argv) {
  std::string filename(argv[1]);
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cout << "Could not open the input file!" << std::endl;
    return 1;
  }

  utils::Buffer file_buffer;
  std::string line;

  for (std::string line; std::getline(file, line);) {
    file_buffer.append_string(line);
    file_buffer.append_utf8('\n');
  }
  file.close();

  auto unit = Compiler::compile(CompilationUnit::Type::Module, filename, file_buffer);
  auto program = unit->ast;

  unit->console.dump_all(std::cerr);

  if (unit->console.has_errors())
    return 1;

  program->dump(std::cout, true);
  unit->ir_module->dump(std::cout);

  return 0;
}

int main(int argc, char** argv) {
  if (argc > 1) {
    return run_file(argc, argv);
  }
  return run_repl();
}
