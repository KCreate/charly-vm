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
#include <thread>

#include <readline/history.h>
#include <readline/readline.h>

#include "charly/charly.h"

#include "charly/utils/buffer.h"
#include "charly/utils/argumentparser.h"

#include "charly/core/compiler.h"
#include "charly/core/compiler/ast.h"
#include "charly/core/compiler/diagnostic.h"
#include "charly/core/compiler/ir/builder.h"

#include "charly/core/runtime/scheduler.h"

using namespace charly;
using namespace charly::core::runtime;
using namespace charly::core::compiler;
using namespace charly::core::compiler::ast;
using namespace charly::core::compiler::ir;

using namespace std::chrono_literals;

extern char** environ;

int run_repl(DiagnosticConsole&) {
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
        std::cout << ".exit       Exit the REPL" << '\n';
        std::cout << ".help       List of meta commands" << '\n';
        continue;
      }

      if (line.compare(".exit") == 0)
        break;
    }

    utils::Buffer source_buffer(line);
    auto unit = Compiler::compile(CompilationUnit::Type::ReplInput, "stdin", source_buffer);

    unit->console.dump_all(std::cerr);
    if (!unit->console.has_errors()) {
      if (utils::ArgumentParser::is_flag_set("dump_ast")) {
        unit->ast->dump(std::cout, print_location);
      }

      if (utils::ArgumentParser::is_flag_set("dump_ir")) {
        unit->ir_module->dump(std::cout);
      }

      if (utils::ArgumentParser::is_flag_set("dump_asm")) {
        unit->bytecode_buffer->dump(std::cout);
      }
    }
  }

  return 0;
}

void run_file(DiagnosticConsole& console, const std::string& filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    console.gerror("Could not open the file: ", filename);
    return;
  }

  utils::Buffer file_buffer;
  std::string line;

  for (std::string line; std::getline(file, line);) {
    file_buffer.write_string(line);
    file_buffer.write_utf8('\n');
  }
  file.close();

  auto unit = Compiler::compile(CompilationUnit::Type::Module, filename, file_buffer);

  unit->console.dump_all(std::cerr);
  if (!unit->console.has_errors()) {
    if (utils::ArgumentParser::is_flag_set("dump_ast")) {
      unit->ast->dump(std::cout, true);
    }

    if (utils::ArgumentParser::is_flag_set("dump_ir")) {
      unit->ir_module->dump(std::cout);
    }

    if (utils::ArgumentParser::is_flag_set("dump_asm")) {
      unit->bytecode_buffer->dump(std::cout);
    }
  }

  auto handler = [](void*) -> void {
    for (int i = 0; i < 5000; i++) {
      safeprint("task % -> % running on fiber %", g_fiber->m_current_task->id, i, g_fiber->id);
      Scheduler::yield();
    }
  };

  for (int i = 0; i < 4; i++) {
    Scheduler::create_task(handler);
  }

  Scheduler::run();
}

void cli(DiagnosticConsole& console) {

  // check for existence of CHARLYVMDIR environment variable
  if (!utils::ArgumentParser::is_env_set("CHARLYVMDIR")) {
    console.gerror("missing 'CHARLYVMDIR' environment variable");
    return;
  }

  // help, version, license
  if (utils::ArgumentParser::is_flag_set("help")) {
    utils::ArgumentParser::print_help(std::cout);
    return;
  }

  if (utils::ArgumentParser::is_flag_set("version")) {
    std::cout << utils::ArgumentParser::VERSION << std::endl;
    return;
  }

  if (utils::ArgumentParser::is_flag_set("license")) {
    std::cout << utils::ArgumentParser::LICENSE << std::endl;
    return;
  }

  // check for filename to execute
  if (utils::ArgumentParser::USER_FLAGS.size() > 0) {
    const std::string& filename = utils::ArgumentParser::USER_FLAGS.front();
    run_file(console, filename);
  } else {
    run_repl(console);
  }
}

int main(int argc, char** argv) {
  utils::ArgumentParser::init_argv(argc, argv);
  utils::ArgumentParser::init_env(environ);
  Scheduler::init();

  utils::Buffer buf("");
  DiagnosticConsole console("charly", buf);
  cli(console);

  console.dump_all(std::cerr);

  return console.has_errors();
}
