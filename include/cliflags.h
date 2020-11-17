/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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
#include <optional>

#pragma once

namespace Charly {
namespace CLIFlags {
  struct FlagDescriptor {
    std::string                name;                      // name of the flag (e.g "dump_asm")
    std::optional<char>        shortselector;             // e.g -h, -v, -l
    std::string                description;               // description of what the flag does
    std::optional<std::string> argument = std::nullopt;   // wether this flag requires an argument
  };

  struct FlagGroup {
    std::string name;
    std::vector<FlagDescriptor> flags;
  };

  extern std::string kUsageMessage;
  extern std::string kExampleUsages;
  extern std::string kLicense;
  extern std::string kVersion;
  extern std::string kEnvironmentStringDelimiter;
  extern std::vector<FlagGroup> kDefinedFlagGroups;

  extern std::unordered_map<std::string, std::vector<std::string>> s_charly_flags;
  extern std::vector<std::string> s_user_flags;
  extern std::unordered_map<std::string, std::string> s_environment;

  void init_argv(int argc, char** argv);
  void init_env(char** environment);

  // Set a flag
  void set_flag(const std::string& name, std::optional<std::string> argument = std::nullopt);

  // Check wether a specific flag is set, addressed by its full name
  bool is_flag_set(const std::string& name);

  // Return all arguments for a specific flag
  const std::vector<std::string> get_arguments_for_flag(const std::string& name);

  // Returns the user argument at index
  const std::optional<std::string> get_argument(uint32_t index);

  // Check wether a flag has a specific argument set
  //
  // e.g:
  // CLIFlags::flag_has_argument("dump", current_filename, true)
  bool flag_has_argument(const std::string& name, const std::string& argument, bool match_substring = false);

  // Return the value of some environment variable
  const std::optional<std::string> get_environment_for_key(const std::string& key);
};
}  // namespace Charly
