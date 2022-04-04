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

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#pragma once

namespace charly::utils {

class ArgumentParser {
public:
  struct FlagDescriptor {
    std::string name;                                    // e.g help
    std::vector<std::string> selectors;                  // e.g --help, -h
    std::string description;                             // description of what the flag does
    std::optional<std::string> argument = std::nullopt;  // wether this flag requires an argument
  };

  struct FlagGroup {
    std::string name;
    std::vector<FlagDescriptor> flags;
  };

  static const std::string USAGE_MESSAGE;
  static const std::string LICENSE;
  static const std::string VERSION;
  static const std::string ENVIRONMENT_STRING_DELIMITER;

  static std::unordered_map<std::string, std::vector<std::string>> CHARLY_FLAGS;
  static std::vector<std::string> USER_FLAGS;
  static std::unordered_map<std::string, std::string> ENVIRONMENT;

  // initialize arguments from C main function arguments and environment pointer
  static void init_argv(int argc, char** argv);
  static void init_env(char** environment);

  // set a flag
  static void set_flag(const std::string& name, std::optional<std::string> argument = std::nullopt);

  // unset a flag
  static void unset_flag(const std::string& name);

  // toggle a flag
  // returns state after toggling
  static bool toggle_flag(const std::string& name);

  // check wether a specific flag is set, addressed by its full name
  static bool is_flag_set(const std::string& name);

  // check wether a specific environment variable exists
  static bool is_env_set(const std::string& name);

  // return all arguments for a specific flag
  static const std::vector<std::string>& get_arguments_for_flag(const std::string& name);

  // returns the user argument at index
  static std::optional<std::string> get_argument(uint32_t index);

  // check wether a flag has a specific argument set
  //
  // e.g:
  // CLIFlags::flag_has_argument("dump", current_filename, true)
  static bool flag_has_argument(const std::string& name, const std::string& argument, bool match_substring = false);

  // return the value of some environment variable
  static std::optional<std::string> get_environment_for_key(const std::string& key);

  // print the help page
  static void print_help(std::ostream& out);
};

}  // namespace charly::utils
