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
#include <unordered_map>

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
  if (vm.real_type(argument) != kTypeString) {
    vm.throw_exception("get_method: expected string");
    return kNull;
  }

  String* arg_string = reinterpret_cast<String*>(argument);
  std::string methodname(arg_string->data, arg_string->length);

  static std::unordered_map<std::string, uintptr_t> method_mapping = {
      {"require", reinterpret_cast<uintptr_t>(Internals::require)},
      {"write", reinterpret_cast<uintptr_t>(Internals::write)},
      {"print", reinterpret_cast<uintptr_t>(Internals::print)},
      {"set_primitive_object", reinterpret_cast<uintptr_t>(Internals::set_primitive_object)},
      {"set_primitive_class", reinterpret_cast<uintptr_t>(Internals::set_primitive_class)},
      {"set_primitive_array", reinterpret_cast<uintptr_t>(Internals::set_primitive_array)},
      {"set_primitive_string", reinterpret_cast<uintptr_t>(Internals::set_primitive_string)},
      {"set_primitive_number", reinterpret_cast<uintptr_t>(Internals::set_primitive_number)},
      {"set_primitive_function", reinterpret_cast<uintptr_t>(Internals::set_primitive_function)},
      {"set_primitive_boolean", reinterpret_cast<uintptr_t>(Internals::set_primitive_boolean)},
      {"set_primitive_null", reinterpret_cast<uintptr_t>(Internals::set_primitive_null)}};

  if (method_mapping.count(methodname) > 0) {
    auto& mapping = method_mapping[methodname];
    return vm.create_cfunction(vm.context.symtable(methodname), 1, mapping);
  }

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

VALUE set_primitive_object(VM& vm, VALUE value) {
  vm.set_primitive_object(value);
  return value;
}

VALUE set_primitive_class(VM& vm, VALUE value) {
  vm.set_primitive_class(value);
  return value;
}

VALUE set_primitive_array(VM& vm, VALUE value) {
  vm.set_primitive_array(value);
  return value;
}

VALUE set_primitive_string(VM& vm, VALUE value) {
  vm.set_primitive_string(value);
  return value;
}

VALUE set_primitive_number(VM& vm, VALUE value) {
  vm.set_primitive_number(value);
  return value;
}

VALUE set_primitive_function(VM& vm, VALUE value) {
  vm.set_primitive_function(value);
  return value;
}

VALUE set_primitive_boolean(VM& vm, VALUE value) {
  vm.set_primitive_boolean(value);
  return value;
}

VALUE set_primitive_null(VM& vm, VALUE value) {
  vm.set_primitive_null(value);
  return value;
}
}  // namespace Internals
}  // namespace Charly
