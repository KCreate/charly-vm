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
#include "charly/core/runtime/allocator.h"
#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/function.h"

using namespace charly;
using namespace charly::core::runtime;
using namespace charly::core::compiler;

using namespace std::chrono_literals;

extern char** environ;

void run_repl(DiagnosticConsole&) {
  bool print_location = false;

  char* buf = nullptr;
  while ((buf = readline("> "))) {
    if (strlen(buf) > 0) {
      add_history(buf);
    }
    std::string line(buf);
    std::free(buf);

    if (line[0] == '.') {
      if (line.compare(".help") == 0) {
        std::cout << ".exit       Exit the REPL" << '\n';
        std::cout << ".help       List of meta commands" << '\n';
        std::cout << ".dump_ast   Toggle AST dump" << '\n';
        std::cout << ".dump_ir    Toggle IR dump" << '\n';
        std::cout << ".dump_asm   Toggle bytecode hex dump" << '\n';
        continue;
      }

      if (line.compare(".exit") == 0)
        break;

      if (line.compare(".dump_ast") == 0) {
        utils::ArgumentParser::toggle_flag("dump_ast");
        continue;
      }

      if (line.compare(".dump_ir") == 0) {
        utils::ArgumentParser::toggle_flag("dump_ir");
        continue;
      }

      if (line.compare(".dump_asm") == 0) {
        utils::ArgumentParser::toggle_flag("dump_asm");
        continue;
      }
    }

    utils::Buffer source_buffer(line);
    auto unit = Compiler::compile("stdin", source_buffer);

    unit->console.dump_all(std::cerr);
    if (!unit->console.has_errors()) {
      if (utils::ArgumentParser::is_flag_set("dump_ast")) {
        unit->ast->dump(std::cout, print_location);
      }

      if (utils::ArgumentParser::is_flag_set("dump_ir")) {
        unit->ir_module->dump(std::cout);
      }

      if (utils::ArgumentParser::is_flag_set("dump_asm")) {
        unit->compiled_module->dump(std::cout);
      }
    }

    if (utils::ArgumentParser::is_flag_set("skipexec")) {
      continue;
    }

    charly::core::runtime::Function* module_function = Scheduler::instance->register_module(unit->compiled_module);
    Fiber* fiber = MemoryAllocator::allocate<Fiber>(module_function);
    fiber->state.acas(Fiber::State::Created, Fiber::State::Ready);
    Scheduler::instance->schedule_fiber(fiber);
  }

  Scheduler::instance->abort(0);
}

void run_file(DiagnosticConsole& console, const std::string& filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    console.gerror("Could not open the file: ", filename);
    Scheduler::instance->abort(1);
    return;
  }

  utils::Buffer file_buffer;
  std::string line;

  for (std::string line; std::getline(file, line);) {
    file_buffer.emit_string(line);
    file_buffer.emit_utf8_cp('\n');
  }
  file.close();

  auto unit = Compiler::compile(filename, file_buffer, CompilationUnit::Type::Module);

  unit->console.dump_all(std::cerr);
  if (unit->console.has_errors()) {
    Scheduler::instance->abort(1);
    return;
  }

  if (utils::ArgumentParser::is_flag_set("dump_ast")) {
    unit->ast->dump(std::cout, true);
  }

  if (utils::ArgumentParser::is_flag_set("dump_ir")) {
    unit->ir_module->dump(std::cout);
  }

  if (utils::ArgumentParser::is_flag_set("dump_asm")) {
    unit->compiled_module->dump(std::cout);
  }

  if (utils::ArgumentParser::is_flag_set("skipexec")) {
    Scheduler::instance->abort(0);
    return;
  }

  charly::core::runtime::Function* module_function = Scheduler::instance->register_module(unit->compiled_module);
  Fiber* main_fiber = MemoryAllocator::allocate<Fiber>(module_function);
  main_fiber->state.acas(Fiber::State::Created, Fiber::State::Ready);
  main_fiber->exit_machine_on_exit = true;
  Scheduler::instance->schedule_fiber(main_fiber);
}

int32_t cli(DiagnosticConsole& console) {

  // check for existence of CHARLYVMDIR environment variable
  if (!utils::ArgumentParser::is_env_set("CHARLYVMDIR")) {
    console.gerror("missing 'CHARLYVMDIR' environment variable");
    return 1;
  }

  // help, version, license
  if (utils::ArgumentParser::is_flag_set("help")) {
    utils::ArgumentParser::print_help(std::cout);
    return 0;
  }

  if (utils::ArgumentParser::is_flag_set("version")) {
    std::cout << utils::ArgumentParser::VERSION << std::endl;
    return 0;
  }

  if (utils::ArgumentParser::is_flag_set("license")) {
    std::cout << utils::ArgumentParser::LICENSE << std::endl;
    return 0;
  }

  Scheduler::initialize();

  // check for filename to execute
  if (utils::ArgumentParser::USER_FLAGS.size() > 0) {
    const std::string& filename = utils::ArgumentParser::USER_FLAGS.front();
    if (filename == "repl") {
      run_repl(console);
    } else {
      run_file(console, filename);
    }
  } else {
    run_repl(console);
  }

  int exit_code = Scheduler::instance->join();
  safeprint("runtime exited");
  return exit_code;
}

int main(int argc, char** argv) {
  utils::ArgumentParser::init_argv(argc, argv);
  utils::ArgumentParser::init_env(environ);

  utils::Buffer buf("");
  DiagnosticConsole console("charly", buf);
  int32_t exit_code = cli(console);
  console.dump_all(std::cerr);

  if (exit_code == 0 && console.has_errors()) {
    exit_code = 1;
  }

  return exit_code;
}
