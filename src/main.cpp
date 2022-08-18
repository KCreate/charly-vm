/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

  if (utils::ArgumentParser::is_flag_set("constants")) {
    debuglnf_notime("# %heap.h%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("kHeapSize                     = %", kHeapSize);
    debuglnf_notime("kHeapRegionSize               = %", kHeapRegionSize);
    debuglnf_notime("kHeapRegionCount              = %", kHeapRegionCount);
    debuglnf_notime("kHeapMinimumMappedRegionCount = %", kHeapMinimumMappedRegionCount);
    debuglnf_notime("kHeapRegionSpanSize           = %", kHeapRegionSpanSize);
    debuglnf_notime("kHeapRegionSpanCount          = %", kHeapRegionSpanCount);
    debuglnf_notime("kHeapRegionUsableSize         = %", kHeapRegionUsableSize);
    debuglnf_notime("kPageSize                     = %", kPageSize);
    debuglnf_notime("kHeapGrowthFactor             = %", kHeapGrowthFactor);
    debuglnf_notime("kHeapShrinkFactor             = %", kHeapShrinkFactor);
    debuglnf_notime("");

    debuglnf_notime("# %interpreter.h%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("kStackOverflowLimit = %", kStackOverflowLimit);
    debuglnf_notime("");

    debuglnf_notime("# %scheduler.h%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("kGlobalRunQueuePriorityChance   = %", kGlobalRunQueuePriorityChance);
    debuglnf_notime("kWorkerMaximumIdleSleepDuration = %", kWorkerMaximumIdleSleepDuration);
    debuglnf_notime("kLocalRunQueueMaxSize           = %", kLocalRunQueueMaxSize);
    debuglnf_notime("kThreadTimeslice                = %", kThreadTimeslice);
    debuglnf_notime("");

    debuglnf_notime("# %thread.h%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("kThreadStackSize = %", kThreadStackSize);
    debuglnf_notime("");

    debuglnf_notime("# %value.h%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("RawInstance::kFieldCount        = %", RawInstance::kFieldCount);
    debuglnf_notime("RawHugeBytes::kFieldCount       = %", RawHugeBytes::kFieldCount);
    debuglnf_notime("RawHugeString::kFieldCount      = %", RawHugeString::kFieldCount);
    debuglnf_notime("RawClass::kFieldCount           = %", RawClass::kFieldCount);
    debuglnf_notime("RawShape::kFieldCount           = %", RawShape::kFieldCount);
    debuglnf_notime("RawFunction::kFieldCount        = %", RawFunction::kFieldCount);
    debuglnf_notime("RawBuiltinFunction::kFieldCount = %", RawBuiltinFunction::kFieldCount);
    debuglnf_notime("RawFiber::kFieldCount           = %", RawFiber::kFieldCount);
    debuglnf_notime("RawFuture::kFieldCount          = %", RawFuture::kFieldCount);
    debuglnf_notime("RawException::kFieldCount       = %", RawException::kFieldCount);
    debuglnf_notime("RawImportException::kFieldCount = %", RawImportException::kFieldCount);
    debuglnf_notime("");

    debuglnf_notime("# %sizeof%", termcolor::yellow, termcolor::reset);
    debuglnf_notime("sizeof(Runtime)    = %", sizeof(Runtime));
    debuglnf_notime("sizeof(Scheduler)  = %", sizeof(Scheduler));
    debuglnf_notime("sizeof(Worker)     = %", sizeof(Worker));
    debuglnf_notime("sizeof(Processor)  = %", sizeof(Processor));
    debuglnf_notime("sizeof(Thread)     = %", sizeof(Thread));
    debuglnf_notime("sizeof(Heap)       = %", sizeof(Heap));
    debuglnf_notime("sizeof(HeapRegion) = %", sizeof(HeapRegion));
    debuglnf_notime("sizeof(Frame)      = %", sizeof(Frame));
    debuglnf_notime("");
  }

  auto start_time = get_steady_timestamp();
  int32_t exit_code = Runtime::run();
  auto end_time = get_steady_timestamp();
  debuglnf("program finished executing in %ms", end_time - start_time);

  return exit_code;
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
