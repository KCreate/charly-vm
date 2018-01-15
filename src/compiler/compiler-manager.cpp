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

#include "compiler-manager.h"

namespace Charly::Compilation {

std::optional<ParserResult> CompilerManager::parse(const std::string& filename, const std::string& source) {
  SourceFile userfile(filename, source);
  Parser parser(userfile);
  ParserResult parse_result = parser.parse();

  // Check for syntax errors
  if (parse_result.syntax_error.has_value()) {
    this->err_stream << parse_result.syntax_error->message << " ";
    parse_result.syntax_error->location.write_to_stream(this->err_stream);
    this->err_stream << '\n';
    return std::nullopt;
  }

  // Dump tokens if the flag was set
  if (this->flags.dump_tokens) {
    if (this->flags.dump_file_contains(filename)) {
      auto& tokens = parse_result.tokens.value();
      for (const auto& token : tokens) {
        token.write_to_stream(this->err_stream);
        this->err_stream << '\n';
      }
    }
  }

  return parse_result;
}

std::optional<CompilerResult> CompilerManager::compile(const std::string& filename, const std::string& source) {
  auto parser_result = this->parse(filename, source);

  if (!parser_result.has_value()) {
    return std::nullopt;
  }

  CompilerConfig cconfig = {.flags = this->flags};
  CompilerContext ccontext(this->symtable, this->stringpool);
  Compiler compiler(ccontext, cconfig);
  CompilerResult compiler_result = compiler.compile(parser_result->abstract_syntax_tree.value());

  if (this->flags.dump_ast) {
    if (this->flags.dump_file_contains(filename)) {
      compiler_result.abstract_syntax_tree->dump(this->err_stream);
    }
  }

  // Print infos, warnings or errors to the console
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
          msg_node->location_start.value().write_to_stream(this->err_stream);
          this->err_stream << ": ";
        }
      }

      this->err_stream << severity << ": " << message.message << '\n';
    }

    if (compiler_result.has_errors) {
      return std::nullopt;
    }
  }

  // Dump a disassembly of the compiled block
  if (this->flags.dump_asm) {
    if (this->flags.dump_file_contains(filename)) {
      Disassembler::Flags disassembler_flags = Disassembler::Flags({.no_branches = this->flags.asm_no_branches,
                                                                    .no_func_branches = this->flags.asm_no_func_branches,
                                                                    .no_offsets = this->flags.asm_no_offsets});
      Disassembler disassembler(compiler_result.instructionblock.value(), disassembler_flags, &ccontext);
      disassembler.dump(this->err_stream);
    }
  }

  return compiler_result;
}

}  // namespace Charly::Compilation
