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

#include "gc.h"
#include "internals.h"
#include "managedcontext.h"
#include "vm.h"

using namespace std;

namespace Charly {
namespace Internals {
VALUE require(VM& vm, VALUE vfilename) {
  // TODO: Deallocate stuff on error

  // Make sure we got a string as filename
  if (vm.real_type(vfilename) != kTypeString) {
    vm.throw_exception("require: expected argument 1 to be a string");
    return kNull;
  }

  String* sfilename = reinterpret_cast<String*>(vfilename);
  std::string filename = std::string(sfilename->data, sfilename->length);

  std::ifstream inputfile(filename);
  if (!inputfile.is_open()) {
    vm.throw_exception("require: could not open " + filename);
    return kNull;
  }
  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());

  auto cresult = vm.context.compiler_manager.compile(filename, source_string);

  if (!cresult.has_value()) {
    vm.throw_exception("require: could not compile " + filename);
    return kNull;
  }

  vm.exec_module(cresult->instructionblock.value());
  return vm.pop_stack();
}

VALUE get_method(VM& vm, VALUE argument) {
  vm.context.out_stream << "Called get_method with ";
  vm.pretty_print(vm.context.out_stream, argument);
  return kNull;
}

VALUE write(VM& vm, VALUE value) {
  vm.pretty_print(vm.context.out_stream, value);
  return kNull;
}

VALUE print(VM& vm, VALUE value) {
  vm.pretty_print(vm.context.out_stream, value);
  vm.context.out_stream << '\n';
  return kNull;
}
}  // namespace Internals
}  // namespace Charly
