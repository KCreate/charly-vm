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

// #include <sstream>

#include <catch2/catch_all.hpp>

#include "charly/utils/buffer.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/compiled_module.h"

using namespace charly;
using namespace charly::utils;
using namespace charly::core::compiler;

TEST_CASE("ir info") {
  auto source_string = "let a = 0 let b = a + 5 foo(->(a) a + b)";
  utils::Buffer source_buffer(source_string);
  auto unit = Compiler::compile("test", source_buffer, CompilationUnit::Type::Module);
  auto module = unit->compiled_module;

  CHECK(module);

  CHECK(module->filename == "test");
  CHECK(module->symbol_table.size() == 3);
  CHECK(module->symbol_table.at(0) == "main");
  CHECK(module->symbol_table.at(1) == "foo");
  CHECK(module->symbol_table.at(2) == "anonymous");

  CHECK(module->function_table.size() == 2);

  auto main = module->function_table.at(0);
  CHECK(main->name == "main");
  CHECK(main->ir_info.valid);
  CHECK(main->ir_info.stacksize == 3);
  CHECK(main->ir_info.local_variables == 1);
  CHECK(main->ir_info.heap_variables == 1);
  CHECK(main->ir_info.argc == 0);
  CHECK(main->ir_info.minargc == 0);
  CHECK(main->ir_info.spread_argument == false);
  CHECK(main->ir_info.arrow_function == false);

  auto anon = module->function_table.at(1);
  CHECK(anon->name == "anonymous");
  CHECK(anon->ir_info.valid);
  CHECK(anon->ir_info.stacksize == 2);
  CHECK(anon->ir_info.local_variables == 1);
  CHECK(anon->ir_info.heap_variables == 0);
  CHECK(anon->ir_info.argc == 1);
  CHECK(anon->ir_info.minargc == 1);
  CHECK(anon->ir_info.spread_argument == false);
  CHECK(anon->ir_info.arrow_function == true);
}

TEST_CASE("exception tables") {
  auto source_string = "try { a } catch(e) { foo(e) } try { a } catch(e) { foo(e) }";
  utils::Buffer source_buffer(source_string);
  auto unit = Compiler::compile("test", source_buffer, CompilationUnit::Type::Module);
  auto module = unit->compiled_module;

  CHECK(module);
  CHECK(module->function_table.size() == 1);
  auto func = module->function_table.at(0);
  CHECK(func->exception_table.size() == 2);
}
