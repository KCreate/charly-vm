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

#include <string_view>

#include "cliflags.h"

namespace Charly {
namespace CLIFlags {

std::string kUsageMessage = "Usage: charly [filename] [flags] [--] [arguments]";

std::string kExampleUsages =
    "Examples:\n"
    "\n"
    "    Executing a file:\n"
    "    $ charly file.ch\n"
    "\n"
    "    Dumping generated bytecodes for a file:\n"
    "    $ charly file.ch -fskipexec -fdump_asm\n"
    "\n"
    "    Dumping generated AST for a file:\n"
    "    $ charly file.ch -fskipexec -fdump_ast\n"
    "\n"
    "    Disabling the cli-parser using '--'\n"
    "    $ charly dump-argv.ch -- -fdump_asm -fskipexec";

std::string kLicense =
    "MIT License \n"
    "\n"
    "Copyright (c) 2017 - 2020 Leonard Schütz \n"
    "\n"
    "Permission is hereby granted, free of charge, to any person obtaining a copy \n"
    "of this software and associated documentation files (the \"Software\"), to deal \n"
    "in the Software without restriction, including without limitation the rights \n"
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"
    "copies of the Software, and to permit persons to whom the Software is \n"
    "furnished to do so, subject to the following conditions: \n"
    "The above copyright notice and this permission notice shall be included in all \n"
    "copies or substantial portions of the Software. \n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE \n"
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE \n"
    "SOFTWARE.";

std::string kVersion = "pre-alpha development release";

std::string kEnvironmentStringDelimiter = "=";

// All existing flags in the charly program
std::vector<FlagGroup> kDefinedFlagGroups = {
  { "Default",                {
    { "help",                 'h',          "Prints the help page" },
    { "version",              'v',          "Prints the version" },
    { "license",              'l',          "Prints the license" },
    { "vmdir",                std::nullopt, "Override CHARLYVMDIR environment variable",            "path" },
    { "skipexec",             std::nullopt, "Don't execute the code" },
    { "skipprelude",          std::nullopt, "Don't execute the prelude" },
  } },
  { "Dump",                   {
    { "dump",                 std::nullopt, "Add file to list of files to be dumped",               "filename" },
    { "dump_ast",             std::nullopt, "Dump the AST of the input file" },
    { "dump_tokens",          std::nullopt, "Dump the tokens of the input file" },
    { "dump_asm",             std::nullopt, "Dump the compiled bytecode of the input file" },
    { "asm_no_offsets",       std::nullopt, "Do not print offsets in dumped bytecode" },
    { "asm_no_branches",      std::nullopt, "Do not print branches in dumped bytecode" },
    { "asm_no_func_branches", std::nullopt, "Do not print function branches in dumped bytecode" }
  } },

#ifndef CHARLY_PRODUCTION
  { "Debugging",              {
    { "instruction_profile",  std::nullopt, "Profile the execution time of individual bytecodes" },
    { "trace_opcodes",        std::nullopt, "Display opcodes as they are executed" },
    { "trace_catchtables",    std::nullopt, "Trace catchtable enter / leave" },
    { "trace_frames",         std::nullopt, "Trace frame enter / leave" },
    { "trace_gc",             std::nullopt, "Display GC debug output" }
  } }
#endif
};

std::unordered_map<std::string, std::vector<std::string>> s_charly_flags;
std::vector<std::string> s_user_flags;
std::unordered_map<std::string, std::string> s_environment;

void init_argv(int argc, char** argv) {
  bool parse_arguments = true;

  for (int argi = 0; argi < argc; argi++) {
    std::string_view arg(argv[argi]);

    // Disable the parsing of builtin arguments after '--' was found
    // in the argument stream
    if (parse_arguments) {

      // Check for short flags
      if (arg.size() == 2) {

        // Disable flag parser after '--' passed
        if (arg == "--") {
          parse_arguments = false;
          continue;
        }

        // Check for short flags
        if (arg[0] == '-') {
          bool found = false;
          for (const FlagGroup& group : kDefinedFlagGroups) {

            for (const FlagDescriptor& flag : group.flags) {
              if (flag.shortselector && flag.shortselector.value() == arg[1]) {
                set_flag(flag.name);
                found = true;
                break;
              }
            }

            if (found)
              break;
          }

          if (!found) {
            s_user_flags.push_back(std::string(arg));
          }

          continue;
        }
      }

      // Check for long flags
      if (arg.size() > 2) {
        if (arg[0] == '-' && arg[1] == '-') {
          std::string_view name = std::string_view(arg).substr(2);

          // Search for long flag
          bool found = false;
          for (const FlagGroup& group : kDefinedFlagGroups) {

            for (const FlagDescriptor& flag : group.flags) {
              if (flag.name == name) {
                set_flag(flag.name);

                if (flag.argument) {

                  // Check if enough arguments are left
                  if (argi + 1 >= argc) {
                    found = true;
                    break;
                  }

                  set_flag(flag.name, argv[++argi]);
                }

                found = true;
              }
            }

            if (found)
              break;
          }

          if (!found) {
            s_user_flags.push_back(std::string(arg));
          }

          continue;
        }
      }
    }

    // skip the executable path
    if (argi != 0) {
      s_user_flags.push_back(std::string(arg));
    }
  }
}

void init_env(char** environment) {
  for (char** current = environment; *current; current++) {
    std::string envstring(*current);
    size_t string_delimiter_index = envstring.find(kEnvironmentStringDelimiter);
    std::string key(envstring, 0, string_delimiter_index);
    std::string value(envstring, string_delimiter_index + 1, std::string::npos);
    s_environment[key] = value;
  }
}

void set_flag(const std::string& name, std::optional<std::string> argument) {
  if (s_charly_flags.count(name) == 0) {
    if (argument) {
      s_charly_flags.insert({name, {argument.value()}});
    } else {
      s_charly_flags.insert({name, {}});
    }
  } else {
    if (argument) {
      s_charly_flags.at(name).push_back(argument.value());
    }
  }
}

bool is_flag_set(const std::string& name) {
  return s_charly_flags.count(name) > 0;
}

const std::vector<std::string> get_arguments_for_flag(const std::string& name) {
  return s_charly_flags.at(name);
}

const std::optional<std::string> get_argument(uint32_t index) {
  if (index < s_user_flags.size()) {
    return s_user_flags.at(index);
  }

  return std::nullopt;
}

bool flag_has_argument(const std::string& name, const std::string& argument, bool match_substring) {
  if (s_charly_flags.count(name) > 0) {
    std::vector<std::string>& flag_arguments = s_charly_flags.at(name);

    for (const std::string& arg : flag_arguments) {
      if (match_substring) {
        if (argument.find(arg) != std::string::npos) {
          return true;
        }
      } else {
        if (arg == argument) {
          return true;
        }
      }
    }
  }

  return false;
}

const std::optional<std::string> get_environment_for_key(const std::string& key) {
  if (s_environment.count(key) > 0) {
    return s_environment.at(key);
  }

  return std::nullopt;
}

} // namespace CLIFlags
} // namespace Charly
