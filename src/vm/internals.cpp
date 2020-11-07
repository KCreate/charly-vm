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

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include "dlfcn.h"

#include "gc.h"
#include "internals.h"
#include "vm.h"

#include "libs/stringbuffer/stringbuffer.h"
#include "libs/sync/sync.h"
#include "libs/math/math.h"
#include "libs/time/time.h"
#include "libs/primitives/array/array.h"
#include "libs/primitives/function/function.h"
#include "libs/primitives/number/number.h"
#include "libs/primitives/object/object.h"
#include "libs/primitives/string/string.h"
#include "libs/primitives/value/value.h"

//        (N)ame
//        (S)ymbol
//     arg(C)
#define DEFINE_INTERNAL_METHOD(N, S, C)                   \
  {                                                       \
    charly_create_symbol(N), {                            \
      N, C, reinterpret_cast<void*>(Charly::Internals::S) \
    }                                                     \
  }

namespace Charly {
namespace Internals {
std::unordered_map<VALUE, MethodSignature> Index::methods = {

    // Primitives
#import "libs/primitives/array/array.def"
#import "libs/primitives/function/function.def"
#import "libs/primitives/number/number.def"
#import "libs/primitives/object/object.def"
#import "libs/primitives/string/string.def"
#import "libs/primitives/value/value.def"

    // Libraries
#import "libs/math/math.def"
#import "libs/time/time.def"
#import "libs/stringbuffer/stringbuffer.def"
#import "libs/sync/sync.def"

    // VM internals
    //
    //                     Symbol                           Function Pointer       ARGC
    DEFINE_INTERNAL_METHOD("charly.vm.import",              import,                2),
    DEFINE_INTERNAL_METHOD("charly.vm.write",               write,                 1),
    DEFINE_INTERNAL_METHOD("charly.vm.getn",                getn,                  0),
    DEFINE_INTERNAL_METHOD("charly.vm.dirname",             dirname,               0),
    DEFINE_INTERNAL_METHOD("charly.vm.exit",                exit,                  1),
    DEFINE_INTERNAL_METHOD("charly.vm.debug_func",          debug_func,            0),
    DEFINE_INTERNAL_METHOD("charly.vm.testfunc",            testfunc,              1),
};

VALUE import(VM& vm, VALUE include, VALUE source) {
  // TODO: Deallocate stuff on error
  CHECK(string, include);
  CHECK(string, source);

  std::string include_filename = charly_string_std(include);
  std::string source_filename = charly_string_std(source);

  // Delete the filename of the source file
  std::string source_folder(source_filename);
  source_folder.erase(source_filename.rfind('/'));

  // Resolve the input file path
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

  // Check if this is a compiled charly library
  // In this case we return an object with all the methods
  // registered in the library
  if (include_filename.size() >= sizeof(".lib")) {

    // Check if we got a *.lib file
    if (!std::strcmp(include_filename.c_str() + include_filename.size() - 4, ".lib")) {
      Immortal<Object> lib = vm.gc.allocate<Object>(8);

      // Check if there is already a resident copy of this library
      //
      // Opening with RTLD_NOLOAD doesn't load the library, it instead returns
      // a library handle if it was already opened before and NULL
      // if it was not
      if (dlopen(include_filename.c_str(), RTLD_NOLOAD)) {
        vm.throw_exception("Can't reopen lib file a second time " + include_filename);
        return kNull;
      }

      void* clib = dlopen(include_filename.c_str(), RTLD_LAZY);

      if (clib == nullptr) {
        vm.throw_exception("Could not open lib file " + include_filename);
        return kNull;
      }

      // Call the constructor of the library
      void (*__charly_constructor)() = reinterpret_cast<void(*)()>(dlsym(clib, "__charly_constructor"));

      if (__charly_constructor) {
        __charly_constructor();
      }

      // Load the function called __charly_signatures
      // It returns a CharlyLibSignatures struct declared in value.h
      //
      // This struct contains the signatures of all the methods in this
      // library
      void* __charly_signatures_ptr = dlsym(clib, "__charly_signatures");
      CharlyLibSignatures* signatures = reinterpret_cast<CharlyLibSignatures*>(__charly_signatures_ptr);
      if (signatures == nullptr) {
        dlclose(clib);
        vm.throw_exception("Could not open library signature section of " + include_filename);
        return kNull;
      }

      uint32_t i = 0;
      while (i < signatures->signatures.size()) {
        std::string name;
        uint32_t argc;
        std::tie(name, argc) = signatures->signatures[i];

        // While we are extracting the method names, we can create
        // CFunction objects for the vm
        void* func_pointer = dlsym(clib, name.c_str());
        if (signatures == nullptr) {
          dlclose(clib);
          vm.throw_exception("Could not read library symbol " + name);
          return kNull;
        }

        VALUE symbol = charly_create_symbol(name);
        lib->write(symbol, vm.gc.allocate<CFunction>(symbol, func_pointer, argc)->as_value());

        i++;
      }

      // Add a CPointer object into the returned object to
      // make sure it gets closed once its not needed anymore
      void (*destructor)(void*) = [](void* clib) {
        void (*__charly_destructor)() = reinterpret_cast<void(*)()>(dlsym(clib, "__charly_destructor"));

        if (__charly_destructor) {
          __charly_destructor();
        }

        dlclose(clib);
      };

      lib->write(SymbolTable::encode("__libptr"), vm.gc.allocate<CPointer>(clib, destructor)->as_value());
      lib->write(SymbolTable::encode("__libpath"), vm.gc.create_string(include_filename));

      return lib->as_value();
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

  return vm.register_module(cresult->instructionblock.value());
}

VALUE write(VM& vm, VALUE value) {
  charly_to_string(vm.context.out_stream, value);
  vm.context.out_stream.flush();

  return kNull;
}

VALUE getn(VM& vm) {
  double num;
  vm.context.in_stream >> num;
  return charly_create_number(num);
}

VALUE dirname(VM& vm) {
  uint8_t* ip = vm.get_ip();
  std::optional<std::string> lookup_result = vm.context.compiler_manager.address_mapping.resolve_address(ip);

  if (!lookup_result) {
    vm.throw_exception("dirname: this address does not belong to a file");
    return kNull;
  }

  std::string filename = lookup_result.value();
  filename.erase(filename.rfind('/'));

  return vm.gc.create_string(filename.c_str(), filename.size());
}

VALUE exit(VM& vm, VALUE status_code) {
  CHECK(number, status_code);
  vm.exit(charly_number_to_uint8(status_code));
  return kNull;
}

VALUE debug_func(VM& vm) {
  Immortal<Object> obj = vm.gc.allocate<Object>(2);

  // Store the sizes of heap types
  Immortal<Object> cell_sizes = vm.gc.allocate<Object>(12);
  cell_sizes->write(SymbolTable::encode("MemoryCell"), charly_create_number(sizeof(MemoryCell)));
  cell_sizes->write(SymbolTable::encode("Header"), charly_create_number(sizeof(Header)));
  cell_sizes->write(SymbolTable::encode("Container"), charly_create_number(sizeof(Container)));
  cell_sizes->write(SymbolTable::encode("Object"), charly_create_number(sizeof(Object)));
  cell_sizes->write(SymbolTable::encode("Array"), charly_create_number(sizeof(Array)));
  cell_sizes->write(SymbolTable::encode("String"), charly_create_number(sizeof(String)));
  cell_sizes->write(SymbolTable::encode("Function"), charly_create_number(sizeof(Function)));
  cell_sizes->write(SymbolTable::encode("CFunction"), charly_create_number(sizeof(CFunction)));
  cell_sizes->write(SymbolTable::encode("Class"), charly_create_number(sizeof(Class)));
  cell_sizes->write(SymbolTable::encode("Frame"), charly_create_number(sizeof(Frame)));
  cell_sizes->write(SymbolTable::encode("CatchTable"), charly_create_number(sizeof(CatchTable)));
  cell_sizes->write(SymbolTable::encode("CPointer"), charly_create_number(sizeof(CPointer)));
  obj->write(SymbolTable::encode("cell_sizes"), cell_sizes);

  // Store the byte lengths of bytecode instructions
  Immortal<Object> bytecode_lengths = vm.gc.allocate<Object>(Opcode::OpcodeCount);
  for (uint8_t i = 0; i < Opcode::OpcodeCount; i++) {
    std::string& mnemonic = kOpcodeMnemonics[i];
    uint32_t length = kInstructionLengths[i];
    bytecode_lengths->write(SymbolTable::encode(mnemonic), charly_create_number(length));
  }
  obj->write(SymbolTable::encode("bytecode_lengths"), bytecode_lengths);

#ifdef CHARLY_PRODUCTION
  obj->write(SymbolTable::encode("production"), kTrue);
#else
  obj->write(SymbolTable::encode("production"), kFalse);
#endif

  return obj->as_value();
}

VALUE testfunc(VM& vm, VALUE argument) {
  CHECK(number, argument);

  Immortal<Object> object = vm.gc.allocate<Object>(2);
  object->write(SymbolTable::encode("a"), argument);
  object->write(SymbolTable::encode("b"), charly_add_number(argument, charly_create_number(1)));
  object->write(SymbolTable::encode("c"), charly_add_number(argument, charly_create_number(2)));
  object->write(SymbolTable::encode("d"), charly_add_number(argument, charly_create_number(3)));
  return object;
}

}  // namespace Internals
}  // namespace Charly
