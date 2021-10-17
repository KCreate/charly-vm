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

#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include <readline/history.h>
#include <readline/readline.h>

#include "charly/charly.h"

#include "charly/utils/buffer.h"
#include "charly/utils/argumentparser.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/runtime.h"
#include "charly/core/runtime/interpreter.h"

using namespace charly;
using namespace charly::core::runtime;
using namespace charly::core::compiler;

using namespace std::chrono_literals;

extern char** environ;

// void run_repl(DiagnosticConsole&) {
//   bool print_location = false;
//
//   char* buf = nullptr;
//   while ((buf = readline("> "))) {
//     if (strlen(buf) > 0) {
//       add_history(buf);
//     }
//     std::string line(buf);
//     std::free(buf);
//
//     if (line[0] == '.') {
//       if (line.compare(".help") == 0) {
//         std::cout << ".exit       Exit the REPL" << '\n';
//         std::cout << ".help       List of meta commands" << '\n';
//         std::cout << ".dump_ast   Toggle AST dump" << '\n';
//         std::cout << ".dump_ir    Toggle IR dump" << '\n';
//         std::cout << ".dump_asm   Toggle bytecode hex dump" << '\n';
//         continue;
//       }
//
//       if (line.compare(".exit") == 0) {
//         Scheduler::instance->abort(0);
//         return;
//       }
//
//       if (line.compare(".dump_ast") == 0) {
//         utils::ArgumentParser::toggle_flag("dump_ast");
//         continue;
//       }
//
//       if (line.compare(".dump_ir") == 0) {
//         utils::ArgumentParser::toggle_flag("dump_ir");
//         continue;
//       }
//
//       if (line.compare(".dump_asm") == 0) {
//         utils::ArgumentParser::toggle_flag("dump_asm");
//         continue;
//       }
//     }
//
//     utils::Buffer source_buffer(line);
//     auto unit = Compiler::compile("stdin", source_buffer);
//
//     unit->console.dump_all(std::cerr);
//     if (!unit->console.has_errors()) {
//       if (utils::ArgumentParser::is_flag_set("dump_ast")) {
//         unit->ast->dump(std::cout, print_location);
//       }
//
//       if (utils::ArgumentParser::is_flag_set("dump_ir")) {
//         unit->ir_module->dump(std::cout);
//       }
//
//       if (utils::ArgumentParser::is_flag_set("dump_asm")) {
//         unit->compiled_module->dump(std::cout);
//       }
//     }
//
//     if (utils::ArgumentParser::is_flag_set("skipexec")) {
//       continue;
//     }
//
//     auto module = unit->compiled_module;
//     Scheduler::instance->register_module(module);
//
//     assert(module->function_table.size());
//     HeapFunction* function = Heap::allocate<HeapFunction>(nullptr, module->function_table.front());
//     assert(function);
//
//     HeapFiber* fiber = Heap::allocate<HeapFiber>(function, false);
//     assert(fiber);
//
//     Scheduler::instance->schedule_thread(fiber);
//   }
// }

// void run_file(DiagnosticConsole& console, const std::string& filename) {
//   std::ifstream file(filename);
//
//   if (!file.is_open()) {
//     console.gerror("Could not open the file: ", filename);
//     Scheduler::instance->abort(1);
//     return;
//   }
//
//   auto unit = Compiler::compile(filename, file, CompilationUnit::Type::Module);
//
//   unit->console.dump_all(std::cerr);
//   if (unit->console.has_errors()) {
//     Scheduler::instance->abort(1);
//     return;
//   }
//
//   if (utils::ArgumentParser::is_flag_set("dump_ast")) {
//     unit->ast->dump(std::cout, true);
//   }
//
//   if (utils::ArgumentParser::is_flag_set("dump_ir")) {
//     unit->ir_module->dump(std::cout);
//   }
//
//   if (utils::ArgumentParser::is_flag_set("dump_asm")) {
//     unit->compiled_module->dump(std::cout);
//   }
//
//   if (utils::ArgumentParser::is_flag_set("skipexec")) {
//     Scheduler::instance->abort(0);
//     return;
//   }
//
//   auto module = unit->compiled_module;
//   Scheduler::instance->register_module(module);
//
//   assert(module->function_table.size());
//   HeapFunction* function = Heap::allocate<HeapFunction>(nullptr, module->function_table.front());
//   assert(function);
//
//   HeapFiber* fiber = Heap::allocate<HeapFiber>(function, true);
//   assert(fiber);
//
//   Scheduler::instance->schedule_thread(fiber);
// }

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

  if (utils::ArgumentParser::is_flag_set("dump_constants")) {
    debuglnf("# %heap.h%", termcolor::yellow, termcolor::reset);
    debuglnf("kHeapTotalSize                      = %", kHeapTotalSize);
    debuglnf("kHeapRegionSize                     = %", kHeapRegionSize);
    debuglnf("kHeapRegionCount                    = %", kHeapRegionCount);
    debuglnf("kHeapRegionPageCount                = %", kHeapRegionPageCount);
    debuglnf("kHeapRegionPossibleObjectAddresses  = %", kHeapRegionPossibleObjectAddresses);
    debuglnf("kHeapInitialMappedRegionCount       = %", kHeapInitialMappedRegionCount);
    debuglnf("");

    debuglnf("# %interpreter.h%", termcolor::yellow, termcolor::reset);
    debuglnf("kStackOverflowLimit = %", kStackOverflowLimit);
    debuglnf("");

    debuglnf("# %scheduler.h%", termcolor::yellow, termcolor::reset);
    debuglnf("kGlobalRunQueuePriorityChance     = %", kGlobalRunQueuePriorityChance);
    debuglnf("kWorkerMaximumIdleSleepDuration   = %", kWorkerMaximumIdleSleepDuration);
    debuglnf("kLocalRunQueueMaxSize             = %", kLocalRunQueueMaxSize);
    debuglnf("kThreadTimeslice                  = %", kThreadTimeslice);
    debuglnf("");

    debuglnf("# %thread.h%", termcolor::yellow, termcolor::reset);
    debuglnf("kThreadStackSize = %", kThreadStackSize);
    debuglnf("");

    debuglnf("# %value.h%", termcolor::yellow, termcolor::reset);
    debuglnf("RawInstance::kSize    = %", RawInstance::kSize);
    debuglnf("RawHugeBytes::kSize   = %", RawHugeBytes::kSize);
    debuglnf("RawHugeString::kSize  = %", RawHugeString::kSize);
    debuglnf("RawFunction::kSize    = %", RawFunction::kSize);
    debuglnf("RawFiber::kSize       = %", RawFiber::kSize);

    return 0;
  }

  Runtime runtime;

  // check for filename to execute
  // if (utils::ArgumentParser::USER_FLAGS.size() > 0) {
  //   const std::string& filename = utils::ArgumentParser::USER_FLAGS.front();
  //   if (filename == "repl") {
  //     run_repl(console);
  //   } else {
  //     run_file(console, filename);
  //   }
  // } else {
  //   run_repl(console);
  // }

  int exit_code = runtime.join();
  debugln("runtime exited");
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
