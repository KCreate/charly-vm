/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
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

#include "libs/math.h"

using namespace std;

namespace Charly {
namespace Internals {
#define ID_TO_STRING(I) #I
#define DEFINE_INTERNAL_METHOD(N, C)                                \
  {                                                                 \
    ID_TO_STRING(N), {                                              \
      ID_TO_STRING(N), C, reinterpret_cast<uintptr_t>(Internals::N) \
    }                                                               \
  }
static std::unordered_map<std::string, InternalMethodSignature> kMethodSignatures = {

// Libs
#import "libs/math.def"

    // VM Barebones
    DEFINE_INTERNAL_METHOD(import, 2),
    DEFINE_INTERNAL_METHOD(write, 1),
    DEFINE_INTERNAL_METHOD(getn, 0),
    DEFINE_INTERNAL_METHOD(set_primitive_object, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_class, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_array, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_string, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_number, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_function, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_generator, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_boolean, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_null, 1),
};

static void test_destructor(uintptr_t data) {
  std::cout << "destructor received: " << (void*)data << std::endl;
}

VALUE import(VM& vm, VALUE include, VALUE source) {
  // TODO: Deallocate stuff on error

  CHECK(string, include);
  CHECK(string, source);

  std::string include_filename = charly_string_std(include);
  std::string source_filename = charly_string_std(source);

  if (include_filename == "__charly_get_debug_cpointer")
    return vm.create_cpointer(0xDEADBEEF69420187,
                              reinterpret_cast<uint64_t>(&(Charly::Internals::test_destructor)));

  // Check if we are importing a standard charly library
  bool import_library = false;
  if (kStandardCharlyLibraries.find(include_filename) != kStandardCharlyLibraries.end()) {
    include_filename = std::string(std::getenv("CHARLYVMDIR")) + "/" + kStandardCharlyLibraries.at(include_filename);
    import_library = true;
  }

  // Delete the filename of the source file
  std::string source_folder(source_filename);
  source_folder.erase(source_filename.rfind('/'));

  // Resolve the input file path
  if (!import_library) {

    // Importing the same file
    if (include_filename == ".") {
      include_filename = source_filename;
    } else {
      // Other include strategies
      switch (include_filename[0]) {

        // Absolute paths
        case '/': {
          // Do nothing to the filepath
          break;
        }

        // Relative paths
        case '.': {
          include_filename = source_folder + "/" + include_filename;
          break;
        }

        // Everything else
        // FIXME: Make sure we catch all edge cases
        default: { include_filename = source_folder + "/" + include_filename; }
      }
    }
  }

  std::ifstream inputfile(include_filename);
  if (!inputfile.is_open()) {
    vm.throw_exception("import: could not open " + include_filename);
    return kNull;
  }
  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());

  auto cresult = vm.context.compiler_manager.compile(include_filename, source_string);

  if (!cresult.has_value()) {
    vm.throw_exception("import: could not compile " + include_filename);
    return kNull;
  }

  vm.exec_module(cresult->instructionblock.value());
  return vm.pop_stack();
}

VALUE get_method(VM& vm, VALUE argument) {
  if (!charly_is_string(argument)) {
    vm.throw_exception("get_method: expected string");
    return kNull;
  }

  char* str_data = charly_string_data(argument);
  uint32_t str_length = charly_string_length(argument);
  std::string methodname(str_data, str_length);

  if (kMethodSignatures.count(methodname) > 0) {
    auto& sig = kMethodSignatures[methodname];
    return vm.create_cfunction(vm.context.symtable(sig.name), sig.argc, sig.func_pointer);
  }

  return kNull;
}

VALUE write(VM& vm, VALUE value) {
  if (charly_is_string(value)) {
    vm.context.out_stream.write(charly_string_data(value), charly_string_length(value));
    return kNull;
  }

  vm.pretty_print(vm.context.out_stream, value);

  return kNull;
}

VALUE getn(VM& vm) {
  double num;
  vm.context.in_stream >> num;
  return charly_create_number(num);
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

VALUE set_primitive_generator(VM& vm, VALUE value) {
  vm.set_primitive_generator(value);
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
