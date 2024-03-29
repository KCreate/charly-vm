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
#include <iostream>
#include <sstream>

#include "charly/utils/argumentparser.h"
#include "charly/utils/colorwriter.h"

namespace charly::utils {

namespace fs = std::filesystem;

const std::string ArgumentParser::LICENSE =
  "MIT License \n"
  "\n"
  "Copyright (c) 2017 - 2022 Leonard Schütz \n"
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
      { "help", { "--help", "-h" }, "Prints the help page" },
      { "version", { "--version", "-v" }, "Prints the version" },
      { "license", { "--license", "-l" }, "Prints the license" },
    } },
  { "Runtime",
    {
      { "maxprocs", { "--maxprocs" }, "Maximum amount of running charly worker threads", "count" },
      { "initial_heap_regions", { "--initial_heap_regions" }, "Initial amount of allocated heap regions", "count" },
      { "skipexec", { "--skipexec" }, "Don't execute input file or REPL input" },
    } },
  { "Debug",
    {
      { "no_ast_opt", { "--no_ast_opt" }, "Disable AST optimizations" },
      { "no_ir_opt", { "--no_ir_opt" }, "Disable IR optimizations" },
      { "ast", { "--ast" }, "Dump processed ASTs" },
      { "ast_raw", { "--ast_raw" }, "Dump unprocessed ASTs" },
      { "ir", { "--ir" }, "Dump the IR generated by the compiler" },
      { "asm", { "--asm" }, "Dump a disassembled view of the bytecode" },
      { "constants", { "--constants" }, "Dump some global constants" },
      { "debug_pattern", { "--debug_pattern" }, "Include files matching the pattern in debug dumps", "pattern" },
      { "validate_heap", { "--validate_heap" }, "Perform heap validation during GC (slow & expensive)" },
    } }
};

std::optional<fs::path> ArgumentParser::USER_FILENAME;
std::unordered_map<std::string, std::vector<std::string>> ArgumentParser::CHARLY_FLAGS;
std::vector<std::string> ArgumentParser::USER_FLAGS;
std::unordered_map<std::string, std::string> ArgumentParser::ENVIRONMENT;

void ArgumentParser::init_argv(int argc, char** argv) {
  bool parse_arguments = true;

  for (int argi = 1; argi < argc; argi++) {
    std::string_view arg(argv[argi]);

    // disable the parsing of builtin arguments after '--' was found
    // in the argument stream
    if (parse_arguments) {
      // stop parsing charly CLI arguments
      if (arg == "--") {
        parse_arguments = false;
        continue;
      }

      // check if the argument matches a known CLI flag
      const FlagDescriptor* found_flag = nullptr;
      for (const FlagGroup& group : kDefinedFlagGroups) {
        for (const FlagDescriptor& flag : group.flags) {
          auto result = std::find(flag.selectors.begin(), flag.selectors.end(), arg);
          if (result != flag.selectors.end()) {
            found_flag = &flag;
            break;
          }
        }

        if (found_flag)
          break;
      }

      if (found_flag) {
        // check if the flag requires an argument
        if (found_flag->argument.has_value()) {
          if (argi + 1 < argc) {
            set_flag(found_flag->name, argv[++argi]);
          }
        } else {
          set_flag(found_flag->name);
        }

        continue;
      }

      // interpret first user argument as a filename
      if (!USER_FILENAME.has_value()) {
        fs::path filename(arg);

        if (!filename.is_absolute()) {
          filename = fs::current_path() / filename;
        }

        USER_FILENAME = filename;
        set_flag("debug_pattern", USER_FILENAME.value());
      }
    }

    USER_FLAGS.emplace_back(arg);
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
      ArgumentParser::CHARLY_FLAGS.insert({ name, { argument.value() } });
    } else {
      ArgumentParser::CHARLY_FLAGS.insert({ name, {} });
    }
  } else {
    if (argument) {
      ArgumentParser::CHARLY_FLAGS.at(name).push_back(argument.value());
    }
  }
}

void ArgumentParser::unset_flag(const std::string& name) {
  ArgumentParser::CHARLY_FLAGS.erase(name);
}

bool ArgumentParser::toggle_flag(const std::string& name) {
  if (ArgumentParser::is_flag_set(name)) {
    ArgumentParser::unset_flag(name);
    return false;
  } else {
    ArgumentParser::set_flag(name);
    return true;
  }
}

bool ArgumentParser::is_flag_set(const std::string& name) {
  return ArgumentParser::CHARLY_FLAGS.count(name);
}

bool ArgumentParser::is_env_set(const std::string& name) {
  return ArgumentParser::ENVIRONMENT.count(name);
}

const std::vector<std::string>& ArgumentParser::get_arguments_for_flag(const std::string& name) {
  return ArgumentParser::CHARLY_FLAGS.at(name);
}

std::optional<std::string> ArgumentParser::get_argument(uint32_t index) {
  if (index < ArgumentParser::USER_FLAGS.size()) {
    return ArgumentParser::USER_FLAGS.at(index);
  }

  return {};
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

std::optional<std::string> ArgumentParser::get_environment_for_key(const std::string& key) {
  if (ArgumentParser::ENVIRONMENT.count(key) > 0) {
    return ArgumentParser::ENVIRONMENT.at(key);
  }

  return {};
}

void ArgumentParser::print_help(std::ostream& out) {
  utils::ColorWriter writer(out);

  writer.fg(Color::Blue, "Usage: ");
  out << "charly [filename] [charly flags] [--] [arguments]"
      << "\n\n";

  for (const FlagGroup& group : kDefinedFlagGroups) {
    writer.fg(Color::Blue, group.name, "\n");

    for (const FlagDescriptor& flag : group.flags) {
      out << "  ";
      for (const std::string& selector : flag.selectors) {
        writer.fg(Color::Yellow, selector);

        if (selector != flag.selectors.back()) {
          out << ", ";
        }
      }

      if (flag.argument) {
        writer.fg(Color::Magenta, " <" + flag.argument.value() + ">");
      }
      out << "\n";

      std::istringstream iss(flag.description);
      for (std::string line; std::getline(iss, line);) {
        out << "      " << line << "\n";
      }

      out << "\n";
    }
  }
}

}  // namespace charly::utils
