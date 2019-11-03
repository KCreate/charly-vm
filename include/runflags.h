/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#pragma once

namespace Charly {
const std::string kEnvironmentStringDelimiter = "=";

struct RunFlags {
  // All arguments and flags passed to the program
  std::vector<std::string> arguments;
  std::vector<std::string> flags;
  std::unordered_map<std::string, std::string> environment;

  // Parsed flags
  bool show_help = false;
  bool show_version = false;
  bool show_license = false;
  bool dump_tokens = false;
  bool dump_ast = false;
  bool dump_asm = false;
  std::vector<std::string> dump_files_include;
  bool asm_no_offsets = false;
  bool asm_no_branches = false;
  bool asm_no_func_branches = false;
  bool skip_execution = false;
  bool instruction_profile = false;
  bool trace_opcodes = false;
  bool trace_catchtables = false;
  bool trace_frames = false;
  bool trace_gc = false;
  bool verbose_addresses = false;
  bool single_worker_thread = false;

  RunFlags(int argc, char** argv, char** envp) {
    // Parse environment variables
    for (char** current = envp; *current; current++) {
      std::string envstring(*current);
      size_t string_delimiter_post = envstring.find(kEnvironmentStringDelimiter);

      std::string key(envstring, 0, string_delimiter_post);
      std::string value(envstring, string_delimiter_post, std::string::npos);

      this->environment[key] = value;
    }

    // If no arguments were passed we can safely skip the rest of this code
    // It only parses arguments and flags
    if (argc == 1)
      return;

    bool append_to_flags = false;
    bool append_to_dump_files_include = false;

    auto append_flag = [&](const std::string& flag) {
      if (!flag.compare("dump_ast"))
        this->dump_ast = true;
      if (!flag.compare("dump_tokens"))
        this->dump_tokens = true;
      if (!flag.compare("dump_asm"))
        this->dump_asm = true;
      if (!flag.compare("asm_no_offsets"))
        this->asm_no_offsets = true;
      if (!flag.compare("asm_no_branches"))
        this->asm_no_branches = true;
      if (!flag.compare("asm_no_func_branches"))
        this->asm_no_func_branches = true;
      if (!flag.compare("skipexec"))
        this->skip_execution = true;
      if (!flag.compare("trace_opcodes"))
        this->trace_opcodes = true;
      if (!flag.compare("trace_catchtables"))
        this->trace_catchtables = true;
      if (!flag.compare("trace_frames"))
        this->trace_frames = true;
      if (!flag.compare("trace_gc"))
        this->trace_gc = true;
      if (!flag.compare("verbose_addresses"))
        this->verbose_addresses = true;
      if (!flag.compare("instruction_profile"))
        this->instruction_profile = true;
      if (!flag.compare("dump_file_include"))
        append_to_dump_files_include = true;
      if (!flag.compare("single_worker_thread"))
        this->single_worker_thread = true;
      this->flags.push_back(flag);
    };

    // Extract all arguments
    for (int argi = 1; argi < argc; argi++) {
      std::string arg(argv[argi]);

      // If this argument comes after a flag, append it to the flags
      //
      // This would be parsed in one step
      // -fexample
      //
      // This would be parsed in two steps
      // -f example
      if (append_to_flags) {
        append_flag(arg);
        append_to_flags = false;
        continue;
      }

      // Add a filename to the dump_file_include list
      //
      // -fdump_file_include file.ch
      if (append_to_dump_files_include) {
        this->dump_files_include.push_back(arg);
        append_to_dump_files_include = false;
        continue;
      }

      // Check if there are enough characters for this argument
      // to be a single character flag
      if (arg.size() == 1) {
        this->arguments.push_back(arg);
        continue;
      }

      // Check single character flags like -h, -v etc.
      if (arg.size() == 2) {
        bool found_flag = false;
        switch (arg[1]) {
          case 'h': {
            this->show_help = true;
            found_flag = true;
            break;
          }
          case 'v': {
            this->show_version = true;
            found_flag = true;
            break;
          }
          case 'l': {
            this->show_license = true;
            found_flag = true;
            break;
          }
          case 'f': {
            append_to_flags = true;
            found_flag = true;
            break;
          }
        }

        if (append_to_flags)
          continue;
        if (found_flag)
          continue;
      }

      // Multiple character flags
      if (arg.size() > 2) {
        if (arg[0] == '-' && arg[1] == '-') {
          bool found_flag = false;

          if (!arg.compare("--help")) {
            this->show_help = true;
            found_flag = true;
          }
          if (!arg.compare("--version")) {
            this->show_version = true;
            found_flag = true;
          }
          if (!arg.compare("--license")) {
            this->show_license = true;
            found_flag = true;
          }
          if (!arg.compare("--flag")) {
            append_to_flags = true;
            found_flag = true;
          }

          if (append_to_flags)
            continue;
          if (found_flag)
            continue;
        }

        if (arg[0] == '-' && arg[1] == 'f') {
          append_flag(arg.substr(2, arg.size()));
          continue;
        }
      }

      this->arguments.push_back(arg);
    }
  }

  inline bool dump_file_contains(const std::string& str) {
    auto& dump_file_list = this->dump_files_include;

    for (auto& name : dump_file_list) {
      if (str.find(name) != std::string::npos) {
        return true;
      }
    }

    return false;
  }
};
}  // namespace Charly
