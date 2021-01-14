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

#include <iostream>
#include <string>

#include <readline/history.h>
#include <readline/readline.h>

#include "charly/core/compiler.h"
#include "charly/utils/buffer.h"

using namespace charly;
using namespace charly::core::compiler;

int main() {
  bool print_location = false;

  char* buf = nullptr;
  while ((buf = readline("> "))) {
    if (strlen(buf) > 0) {
      add_history(buf);
    }
    std::string line(buf);
    free(buf);

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

    {
      utils::Buffer buf;
      buf.append_string(line);

      try {
        Parser parser("stdin", buf.buffer_string());
        ast::ref<ast::Program> program = parser.parse_program();

        // check if program has nodes
        if (ref<Block> body = cast<Block>(program->body)) {
          if (body->statements.size() == 0)
            continue;
        }

        DumpPass(std::cout, print_location).visit(program);
      } catch (CompilerError& exc) { std::cout << exc << '\n'; }
    }
  }

  return 0;
}
