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

#include <readline/history.h>
#include <readline/readline.h>

#include "charly/charly.h"

#include "charly/utils/argumentparser.h"
#include "charly/utils/buffer.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/interpreter.h"
#include "charly/core/runtime/runtime.h"

using namespace charly;
using namespace charly::core::runtime;
using namespace charly::core::compiler;

using namespace std::chrono_literals;

extern char** environ;

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
    debuglnf("kPageSize                           = %", kPageSize);
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
    debuglnf("RawClass::kSize       = %", RawClass::kSize);
    debuglnf("RawShape::kSize       = %", RawShape::kSize);
    debuglnf("RawFunction::kSize    = %", RawFunction::kSize);
    debuglnf("RawFiber::kSize       = %", RawFiber::kSize);
    debuglnf("");

    debuglnf("# %sizeof%", termcolor::yellow, termcolor::reset);
    debuglnf("sizeof(Runtime)       = %", sizeof(Runtime));
    debuglnf("sizeof(Scheduler)     = %", sizeof(Scheduler));
    debuglnf("sizeof(Worker)        = %", sizeof(Worker));
    debuglnf("sizeof(Processor)     = %", sizeof(Processor));
    debuglnf("sizeof(Thread)        = %", sizeof(Thread));
    debuglnf("sizeof(Heap)          = %", sizeof(Heap));
    debuglnf("sizeof(HeapRegion)    = %", sizeof(HeapRegion));
    debuglnf("sizeof(Frame)         = %", sizeof(Frame));
    debuglnf("");
  }

  return Runtime::run();
}

int main(int argc, char** argv) {
  // readline.h setup
  rl_initialize();
  using_history();

  // disable filename autocompletion with TAB
  rl_bind_key('\t', rl_insert);

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
