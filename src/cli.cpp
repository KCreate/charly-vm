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

#include <fstream>
#include <iostream>

#include "charly.h"
#include "cli.h"
#include "compiler.h"
#include "disassembler.h"
#include "parser.h"
#include "sourcefile.h"

namespace Charly {
using namespace Compilation;

int CLI::run() {
  if (this->flags.show_help) {
    std::cout << kHelpMessage << '\n';
    return 0;
  }

  if (this->flags.show_license) {
    std::cout << kLicense << '\n';
    return 0;
  }

  if (this->flags.show_version) {
    std::cout << kVersion << '\n';
    return 0;
  }

  // Check if a filename was given
  if (this->flags.arguments.size() == 0) {
    std::cerr << "No filename given!" << '\n';
    std::cout << kHelpMessage << '\n';
    return 1;
  }

  // Read the userfile
  std::ifstream inputfile(this->flags.arguments[0]);
  if (!inputfile.is_open()) {
    std::cerr << "Could not open file" << '\n';
    return 1;
  }
  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());
  SourceFile userfile(this->flags.arguments[0], source_string);

  // Parse the userfile
  Parser parser(userfile);
  ParserResult parse_result = parser.parse();

  // Check for syntax errors
  if (parse_result.syntax_error.has_value()) {
    std::cerr << parse_result.syntax_error->message << " ";
    parse_result.syntax_error->location.write_to_stream(std::cerr);
    std::cerr << '\n';
    return 1;
  }

  // Dump tokens if the flag was set
  if (this->flags.dump_tokens) {
    auto& tokens = parse_result.tokens.value();
    for (const auto& token : tokens) {
      token.write_to_stream(std::cout);
      std::cout << '\n';
    }
  }

  // Create the compiler
  CompilerConfig cconfig = {
      .known_top_level_constants = {"require_no_exec", "require", "Object", "Class", "Array",  "String", "Number",
                                    "Function",        "Boolean", "Null",   "stdin", "stdout", "stderr", "print",
                                    "write",           "gets",    "getc",   "exit",  "sleep",  "Charly"},
      .wrap_inclusion_function = true,
      .flags = this->flags};
  CompilerContext compiler_context;
  Compiler compiler(compiler_context, cconfig);

  // Compile the userfile
  CompilerResult compiler_result = compiler.compile(parse_result.abstract_syntax_tree.value());

  // Dump the ast if the flag was set
  if (this->flags.dump_ast) {
    compiler_result.abstract_syntax_tree->dump(std::cout);
  }

  // Show any infos, warnings or errors
  if (compiler_result.messages.size() > 0) {
    for (const auto& message : compiler_result.messages) {
      std::string severity;

      switch (message.severity) {
        case CompilerMessage::Severity::Info: {
          severity = "info";
          break;
        }
        case CompilerMessage::Severity::Warning: {
          severity = "warning";
          break;
        }
        case CompilerMessage::Severity::Error: {
          severity = "error";
          break;
        }
      }

      if (message.node.has_value()) {
        AST::AbstractNode* msg_node = message.node.value();

        if (msg_node->location_start.has_value()) {
          msg_node->location_start.value().write_to_stream(std::cerr);
          std::cerr << ": ";
        }
      }

      std::cerr << severity << ": " << message.message << '\n';
    }

    if (compiler_result.has_errors) {
      return 1;
    }
  }

  // Dump a disassembly of the compiled block
  if (this->flags.dump_asm) {
    Disassembler::Flags disassembler_flags = Disassembler::Flags({.no_branches = this->flags.asm_no_branches,
                                                                  .no_func_branches = this->flags.asm_no_func_branches,
                                                                  .no_offsets = this->flags.asm_no_offsets});
    Disassembler disassembler(compiler_result.instructionblock.value(), disassembler_flags, &compiler_context);
    disassembler.dump(std::cout);
  }

  if (this->flags.skip_execution) {
    return 0;
  }

  GarbageCollector gc(
      {.initial_heap_count = 8, .heap_cell_count = 512, .heap_growth_factor = 1.5, .trace = this->flags.trace_gc});
  VMContext context({.symtable = compiler_context.symtable,
                     .stringpool = compiler_context.stringpool,
                     .gc = gc,
                     .instruction_profile = this->flags.instruction_profile,
                     .trace_opcodes = this->flags.trace_opcodes,
                     .trace_catchtables = this->flags.trace_catchtables,
                     .trace_frames = this->flags.trace_frames,
                     .out_stream = std::cout,
                     .err_stream = std::cerr});
  VM vm(context);
  vm.exec_module(compiler_result.instructionblock.value());

  // Display the instruction profile if requested
  if (this->flags.instruction_profile) {
    uint8_t opcode = 0;
    std::cout << "Instruction Profile:" << '\n';
    while (opcode < kOpcodeCount) {
      std::cout << std::setw(26);
      std::cout << kOpcodeMnemonics[opcode] << ": ";
      std::cout << std::setw(1);
      std::cout << vm.instruction_profile.entries[opcode].average_length << " nanoseconds ";
      std::cout << "(" << vm.instruction_profile.entries[opcode].encountered << " samples)" << '\n';
      opcode++;
    }
  }

  return 0;
}
}  // namespace Charly
