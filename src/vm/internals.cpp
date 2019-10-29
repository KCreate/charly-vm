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
#include "dlfcn.h"

#include "gc.h"
#include "internals.h"
#include "managedcontext.h"
#include "vm.h"

#include "libs/math/math.h"
#include "libs/primitives/array.h"

using namespace std;

namespace Charly {
namespace Internals {
#define ID_TO_STRING(I) #I
#define DEFINE_INTERNAL_METHOD(N, C)                                \
  {                                                                 \
    ID_TO_STRING(N), {                                              \
      ID_TO_STRING(N), C, reinterpret_cast<void*>(Internals::N) \
    }                                                               \
  }
static std::unordered_map<std::string, InternalMethodSignature> kMethodSignatures = {

// Libs
#import "libs/primitives/array.def"
#import "libs/math/math.def"

    // VM Barebones
    DEFINE_INTERNAL_METHOD(import, 2),
    DEFINE_INTERNAL_METHOD(write, 1),
    DEFINE_INTERNAL_METHOD(getn, 0),
    DEFINE_INTERNAL_METHOD(dirname, 0),

    DEFINE_INTERNAL_METHOD(set_primitive_object, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_class, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_array, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_string, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_number, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_function, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_generator, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_boolean, 1),
    DEFINE_INTERNAL_METHOD(set_primitive_null, 1),

    DEFINE_INTERNAL_METHOD(defer, 2),
    DEFINE_INTERNAL_METHOD(defer_interval, 2),
    DEFINE_INTERNAL_METHOD(clear_timer, 1),
    DEFINE_INTERNAL_METHOD(clear_interval, 1),
    DEFINE_INTERNAL_METHOD(exit, 1),

    DEFINE_INTERNAL_METHOD(register_worker_task, 2),
};

VALUE import(VM& vm, VALUE include, VALUE source) {
  // TODO: Deallocate stuff on error

  CHECK(string, include);
  CHECK(string, source);

  std::string include_filename = charly_string_std(include);
  std::string source_filename = charly_string_std(source);

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
      Object* lib = charly_as_object(lalloc.create_object(8));

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

VALUE defer(VM& vm, VALUE cb, VALUE dur) {
  CHECK(function, cb);
  CHECK(number, dur);

  uint32_t ms = charly_number_to_uint32(dur);

  Timestamp now = std::chrono::steady_clock::now();
  Timestamp exec_at = now + std::chrono::milliseconds(ms);

  return charly_create_integer(vm.register_timer(exec_at, VMTask(cb, kNull)));
}

VALUE defer_interval(VM& vm, VALUE cb, VALUE period) {
  CHECK(function, cb);
  CHECK(number, period);

  uint32_t ms = charly_number_to_uint32(period);

  return charly_create_integer(vm.register_interval(ms, VMTask(cb, kNull)));
}

VALUE clear_timer(VM&vm, VALUE uid) {
  CHECK(number, uid);
  vm.clear_timer(charly_number_to_uint64(uid));
  return kNull;
}

VALUE clear_interval(VM&vm, VALUE uid) {
  CHECK(number, uid);
  vm.clear_interval(charly_number_to_uint64(uid));
  return kNull;
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
