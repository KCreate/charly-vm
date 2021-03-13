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

#include <iostream>

#include "charly/utils/argumentparser.h"
#include "charly/utils/colorwriter.h"

namespace charly::utils {

using FlagDescriptor = ArgumentParser::FlagDescriptor;
using FlagGroup = ArgumentParser::FlagGroup;

const std::string ArgumentParser::USAGE_MESSAGE = "Usage: charly [filename] [flags] [--] [arguments]";

  const std::string ArgumentParser::LICENSE =
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

const std::string ArgumentParser::VERSION = "pre-alpha development release";

const std::string ArgumentParser::ENVIRONMENT_STRING_DELIMITER = "=";

const std::vector<ArgumentParser::FlagGroup> kDefinedFlagGroups = {
  { "Default",
    {
      { "help",        'h',          "Prints the help page" },
      { "version",     'v',          "Prints the version" },
      { "license",     'l',          "Prints the license" },
    } },
  { "Dump",
    {
      { "dump_ast",    std::nullopt, "Dump the AST of the input file" },
      { "dump_asm",    std::nullopt, "Dump the compiled bytecode of the input file" },
    } }
};

std::unordered_map<std::string, std::vector<std::string>> ArgumentParser::CHARLY_FLAGS;
std::vector<std::string> ArgumentParser::USER_FLAGS;
std::unordered_map<std::string, std::string> ArgumentParser::ENVIRONMENT;

void ArgumentParser::init_argv(int argc, char** argv) {
  bool parse_arguments = true;

  for (int argi = 0; argi < argc; argi++) {
    std::string_view arg(argv[argi]);

    // disable the parsing of builtin arguments after '--' was found
    // in the argument stream
    if (parse_arguments) {

      // check for short flags
      if (arg.size() == 2) {

        // disable flag parser after '--' passed
        if (arg == "--") {
          parse_arguments = false;
          continue;
        }

        // check for short flags
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
            USER_FLAGS.push_back(std::string(arg));
          }

          continue;
        }
      }

      // check for long flags
      if (arg.size() > 2) {
        if (arg[0] == '-' && arg[1] == '-') {
          std::string_view name = std::string_view(arg).substr(2);

          // search for long flag
          bool found = false;
          for (const FlagGroup& group : kDefinedFlagGroups) {

            for (const FlagDescriptor& flag : group.flags) {
              if (flag.name == name) {
                set_flag(flag.name);

                if (flag.argument) {

                  // check if enough arguments are left
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
            USER_FLAGS.push_back(std::string(arg));
          }

          continue;
        }
      }
    }

    // skip the executable path
    if (argi != 0) {
      USER_FLAGS.push_back(std::string(arg));
    }
  }
}

void ArgumentParser::init_env(char** environment) {
  for (char** current = environment; *current; current++) {
    std::string envstring(*current);
    size_t string_delimiter_index = envstring.find(ArgumentParser::ENVIRONMENT_STRING_DELIMITER);
    std::string key(envstring, 0, string_delimiter_index);
    std::string value(envstring, string_delimiter_index + 1, std::string::npos);
    ArgumentParser::ENVIRONMENT[key] = value;
  }
}

void ArgumentParser::set_flag(const std::string& name, std::optional<std::string> argument) {
  if (ArgumentParser::CHARLY_FLAGS.count(name) == 0) {
    if (argument) {
      ArgumentParser::CHARLY_FLAGS.insert({name, {argument.value()}});
    } else {
      ArgumentParser::CHARLY_FLAGS.insert({name, {}});
    }
  } else {
    if (argument) {
      ArgumentParser::CHARLY_FLAGS.at(name).push_back(argument.value());
    }
  }
}

bool ArgumentParser::is_flag_set(const std::string& name) {
  return ArgumentParser::CHARLY_FLAGS.count(name);
}

bool ArgumentParser::is_env_set(const std::string& name) {
  return ArgumentParser::ENVIRONMENT.count(name);
}

const std::vector<std::string> ArgumentParser::get_arguments_for_flag(const std::string& name) {
  return ArgumentParser::CHARLY_FLAGS.at(name);
}

const std::optional<std::string> ArgumentParser::get_argument(uint32_t index) {
  if (index < ArgumentParser::USER_FLAGS.size()) {
    return ArgumentParser::USER_FLAGS.at(index);
  }

  return std::nullopt;
}

bool ArgumentParser::flag_has_argument(const std::string& name, const std::string& argument, bool match_substring) {
  if (ArgumentParser::CHARLY_FLAGS.count(name) > 0) {
    std::vector<std::string>& flag_arguments = ArgumentParser::CHARLY_FLAGS.at(name);

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

const std::optional<std::string> ArgumentParser::get_environment_for_key(const std::string& key) {
  if (ArgumentParser::ENVIRONMENT.count(key) > 0) {
    return ArgumentParser::ENVIRONMENT.at(key);
  }

  return std::nullopt;
}

void ArgumentParser::print_help(std::ostream& out) {
  out << USAGE_MESSAGE << "\n";
  out << "\n";

  for (const FlagGroup& group : kDefinedFlagGroups) {
    out << group.name << "\n";

    for (const FlagDescriptor& flag : group.flags) {
      out << "  ";
      if (flag.shortselector) {
        out << "-" << flag.shortselector.value();
        out << ", ";
      }
      out << "--" << flag.name;
      if (flag.argument) {
        out << " <" + flag.argument.value() + ">";
      }
      out << "\n";
      out << "      " << flag.description;
      out << "\n";
      out << "\n";
    }
  }
}

}  // namespace charly::utils
