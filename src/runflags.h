/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <vector>
#include <unordered_map>

#pragma once

namespace Charly {
  const std::string kEnvironmentStringDelimiter = "=";

  struct RunFlags {

    // All arguments and flags passed to the program
    std::vector<std::string> arguments;
    std::vector<std::string> flags;
    std::unordered_map<std::string, std::string> environment;

    // Parsed flags
    bool show_help;
    bool show_version;
    bool show_license;
    bool dump_tokens;
    bool dump_ast;
    bool skip_execution;

    RunFlags(int argc, char** argv, char** envp) {

      // Default initialize all flags
      this->show_help = false;
      this->show_version = false;
      this->show_license = false;
      this->dump_tokens = false;
      this->dump_ast = false;
      this->skip_execution = false;

      bool append_to_flags = false;

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
          this->flags.push_back(arg);
          append_to_flags = false;
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

          if (append_to_flags) continue;
          if (found_flag) continue;
        }

        // Multiple character flags
        if (arg.size() > 2) {
          if (arg[0] == '-' && arg[1] == '-') {
            bool found_flag = false;

            if (!arg.compare("--help")) this->show_help = true; found_flag = true;
            if (!arg.compare("--version")) this->show_version = true; found_flag = true;
            if (!arg.compare("--license")) this->show_license = true; found_flag = true;
            if (!arg.compare("--flag")) append_to_flags = true; found_flag = true;

            if (append_to_flags) continue;
            if (found_flag) continue;
          }

          if (arg[0] == '-' && arg[1] == 'f') {
            this->flags.push_back(arg.substr(1, arg.size()));
            continue;
          }
        }

        this->arguments.push_back(arg);
      }

      // Parse environment variables
      for (char** current = envp; *current; current++) {
        std::string envstring(*current);

        std::string key = envstring.substr(0, envstring.find(kEnvironmentStringDelimiter));
        std::string value = envstring.substr(envstring.find(kEnvironmentStringDelimiter) + 1, envstring.size());

        this->environment[key] = value;
      }

    }
  };
}
