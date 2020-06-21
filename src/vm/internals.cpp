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
#include "managedcontext.h"
#include "vm.h"

#include "libs/buffer/buffer.h"
#include "libs/defer/defer.h"
#include "libs/math/math.h"
#include "libs/time/time.h"
#include "libs/primitives/value/value.h"
#include "libs/primitives/array/array.h"
#include "libs/primitives/function/function.h"
#include "libs/primitives/object/object.h"
#include "libs/primitives/string/string.h"

#define DEFINE_INTERNAL_METHOD(N, S, C)                   \
  {                                                       \
    charly_create_symbol(N), {                                                  \
      N, C, reinterpret_cast<void*>(Charly::Internals::S) \
    }                                                     \
  }

namespace Charly {
namespace Internals {
std::unordered_map<VALUE, MethodSignature> Index::methods = {

    // Primitives
#import "libs/primitives/array/array.def"
#import "libs/primitives/function/function.def"
#import "libs/primitives/object/object.def"
#import "libs/primitives/string/string.def"
#import "libs/primitives/value/value.def"

    // Libraries
#import "libs/math/math.def"
#import "libs/time/time.def"
#import "libs/buffer/buffer.def"
#import "libs/defer/defer.def"

    // VM internals
    DEFINE_INTERNAL_METHOD("charly.vm.import", import, 2),
    DEFINE_INTERNAL_METHOD("charly.vm.write", write, 1),
    DEFINE_INTERNAL_METHOD("charly.vm.getn", getn, 0),
    DEFINE_INTERNAL_METHOD("charly.vm.dirname", dirname, 0),
    DEFINE_INTERNAL_METHOD("charly.vm.exit", exit, 1),
    DEFINE_INTERNAL_METHOD("charly.vm.register_worker_task", register_worker_task, 2),
};

// Standard charly libraries
std::unordered_map<std::string, std::string> Index::standard_libraries = {

    // Internal primitives
    {"_charly_array", "src/stdlib/primitives/array.ch"},
    {"_charly_value", "src/stdlib/primitives/value.ch"},
    {"_charly_boolean", "src/stdlib/primitives/boolean.ch"},
    {"_charly_class", "src/stdlib/primitives/class.ch"},
    {"_charly_function", "src/stdlib/primitives/function.ch"},
    {"_charly_generator", "src/stdlib/primitives/generator.ch"},
    {"_charly_null", "src/stdlib/primitives/null.ch"},
    {"_charly_number", "src/stdlib/primitives/number.ch"},
    {"_charly_object", "src/stdlib/primitives/object.ch"},
    {"_charly_string", "src/stdlib/primitives/string.ch"},

    // Helper stuff
    {"_charly_defer", "src/stdlib/libs/defer.ch"},

    // Libraries
    {"_charly_math", "src/stdlib/libs/math.ch"},
    {"_charly_time", "src/stdlib/libs/time.ch"},
    {"_charly_heap", "src/stdlib/libs/heap.ch"},
    {"_charly_unittest", "src/stdlib/libs/unittest.ch"}
};

VALUE import(VM& vm, VALUE include, VALUE source) {
  // TODO: Deallocate stuff on error

  CHECK(string, include);
  CHECK(string, source);

  std::string include_filename = charly_string_std(include);
  std::string source_filename = charly_string_std(source);

  // Check if we are importing a standard charly library
  bool import_library = false;
  auto& std_libs = Index::standard_libraries;
  if (std_libs.find(include_filename) != std_libs.end()) {
    include_filename = std::string(std::getenv("CHARLYVMDIR")) + "/" + std_libs.at(include_filename);
    import_library = true;
  }

  // Delete the filename of the source file
  std::string source_folder(source_filename);
  source_folder.erase(source_filename.rfind('/'));

  // Resolve the input file path
  if (!import_library) {

    // Importing the same file
    if (include_filename == ".") {
      vm.throw_exception("import: could not open '.'");
      return kNull;
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

  // Check if this is a compiled charly library
  // In this case we return an object with all the methods
  // registered in the library
  if (include_filename.size() >= sizeof(".lib")) {

    // Check if we got a *.lib file
    if (!std::strcmp(include_filename.c_str() + include_filename.size() - 4, ".lib")) {
      ManagedContext lalloc(vm);
      Charly::Object* lib = charly_as_object(lalloc.create_object(8));

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
        CFunction* cfunc = charly_as_cfunction(lalloc.create_cfunction(
          charly_create_symbol(name),
          argc,
          dlsym(clib, name.c_str())
        ));

        lib->container->insert({vm.context.symtable(name), charly_create_pointer(cfunc)});

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
      lib->container->insert({
        vm.context.symtable("__libptr"),
        lalloc.create_cpointer(clib, reinterpret_cast<void*>(destructor))
      });

      // Insert the path of the library into the object
      lib->container->insert({
        vm.context.symtable("__libpath"),
        lalloc.create_string(include_filename)
      });

      return charly_create_pointer(lib);
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

  Function* fn = charly_as_function(vm.register_module(cresult->instructionblock.value()));
  return vm.exec_module(fn);
}

VALUE write(VM& vm, VALUE value) {
  vm.to_s(vm.context.out_stream, value);

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

  return vm.create_string(filename.c_str(), filename.size());
}

VALUE exit(VM& vm, VALUE status_code) {
  CHECK(number, status_code);
  vm.exit(charly_number_to_uint8(status_code));
  return kNull;
}

VALUE register_worker_task(VM& vm, VALUE v, VALUE cb) {
  CHECK(function, cb);
  AsyncTask task = {AsyncTaskType::fs_access, {}, cb};
  task.arguments.push_back(v);
  vm.register_worker_task(task);
  return kNull;
}

}  // namespace Internals
}  // namespace Charly
