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
#include "libs/primitives/array/array.h"
#include "libs/primitives/function/function.h"
#include "libs/primitives/number/number.h"
#include "libs/primitives/object/object.h"
#include "libs/primitives/string/string.h"
#include "libs/primitives/value/value.h"

//        (N)ame
//        (S)ymbol
//     arg(C)
// thread (P)olicy
#define DEFINE_INTERNAL_METHOD(N, S, C, P)                   \
  {                                                          \
    charly_create_symbol(N), {                               \
      N, C, reinterpret_cast<void*>(Charly::Internals::S), P \
    }                                                        \
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
#import "libs/buffer/buffer.def"
#import "libs/defer/defer.def"

    // VM internals
    //
    //                     Symbol                           Function Pointer       ARGC   Thread-policy
    DEFINE_INTERNAL_METHOD("charly.vm.import",              import,                2,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.write",               write,                 1,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.getn",                getn,                  0,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.dirname",             dirname,               0,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.exit",                exit,                  1,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.get_argv",            get_argv,              0,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.get_environment",     get_environment,       0,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.get_active_frame",    get_active_frame,      0,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.get_parent_frame",    get_parent_frame,      1,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.get_block_address",   get_block_address,     1,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.resolve_address",     resolve_address,       1,     kThreadMain),
    DEFINE_INTERNAL_METHOD("charly.vm.debug_func",          debug_func,            1,     kThreadBoth),
};

// Standard charly libraries
std::unordered_map<std::string, std::string> Index::standard_libraries = {

    // Internal primitives
    {"_charly_array", "src/stdlib/primitives/array.ch"},
    {"_charly_boolean", "src/stdlib/primitives/boolean.ch"},
    {"_charly_class", "src/stdlib/primitives/class.ch"},
    {"_charly_function", "src/stdlib/primitives/function.ch"},
    {"_charly_generator", "src/stdlib/primitives/generator.ch"},
    {"_charly_null", "src/stdlib/primitives/null.ch"},
    {"_charly_number", "src/stdlib/primitives/number.ch"},
    {"_charly_object", "src/stdlib/primitives/object.ch"},
    {"_charly_string", "src/stdlib/primitives/string.ch"},
    {"_charly_value", "src/stdlib/primitives/value.ch"},

    // Libraries
    {"_charly_defer", "src/stdlib/libs/defer.ch"},
    {"_charly_error", "src/stdlib/libs/error.ch"},
    {"_charly_heap", "src/stdlib/libs/heap.ch"},
    {"_charly_math", "src/stdlib/libs/math.ch"},
    {"_charly_time", "src/stdlib/libs/time.ch"},
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
        uint8_t thread_policy;
        std::tie(name, argc, thread_policy) = signatures->signatures[i];

        // While we are extracting the method names, we can create
        // CFunction objects for the vm
        void* func_pointer = dlsym(clib, name.c_str());
        CFunction* cfunc = charly_as_cfunction(lalloc.create_cfunction(
          charly_create_symbol(name),
          argc,
          func_pointer,
          thread_policy
        ));

        lib->container->insert({charly_create_symbol(name), charly_create_pointer(cfunc)});

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
        SymbolTable::encode("__libptr"),
        lalloc.create_cpointer(clib, reinterpret_cast<void*>(destructor))
      });

      // Insert the path of the library into the object
      lib->container->insert({
        SymbolTable::encode("__libpath"),
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

  return vm.create_string(filename.c_str(), filename.size());
}

VALUE exit(VM& vm, VALUE status_code) {
  CHECK(number, status_code);
  vm.exit(charly_number_to_uint8(status_code));
  return kNull;
}

VALUE get_argv(VM& vm) {

  // Check for nullptr
  if (!vm.context.argv) return kNull;
  std::vector<std::string>& argv = (* vm.context.argv);

  // Allocate result array
  ManagedContext lalloc(vm);
  Array* argv_array = charly_as_array(lalloc.create_array(argv.size()));

  // Append argv elements
  for (const std::string& argument : argv) {
    argv_array->data->push_back(lalloc.create_string(argument));
  }

  return charly_create_pointer(argv_array);
}

VALUE get_environment(VM& vm) {

  // Check for nullptr
  if (!vm.context.environment) return kNull;
  std::unordered_map<std::string, std::string>& environment = (* vm.context.environment);

  // Allocate result object
  ManagedContext lalloc(vm);
  Object* environment_obj = charly_as_object(lalloc.create_object(environment.size()));

  // Append environment variables
  for (const auto& entry : environment) {
    environment_obj->container->insert({
      SymbolTable::encode(entry.first),
      lalloc.create_string(entry.second)
    });
  }

  return charly_create_pointer(environment_obj);
}

VALUE get_active_frame(VM& vm) {
  ManagedContext lalloc(vm);

  // Acquire frame information
  Frame* vm_frame      = vm.get_current_frame();
  VALUE frame_id       = charly_create_pointer(vm_frame);
  VALUE caller_value   = vm_frame->caller_value;
  VALUE self_value     = vm_frame->self;
  VALUE origin_address = charly_create_integer(reinterpret_cast<size_t>(vm_frame->origin_address));

  // Create stacktrace entry
  Object* obj = charly_as_object(lalloc.create_object(3));
  obj->container->insert({SymbolTable::encode("id"),             frame_id});
  obj->container->insert({SymbolTable::encode("caller"),         caller_value});
  obj->container->insert({SymbolTable::encode("self_value"),     self_value});
  obj->container->insert({SymbolTable::encode("origin_address"), origin_address});

  return charly_create_pointer(obj);
}

VALUE get_parent_frame(VM& vm, VALUE frame_ref) {
  CHECK(frame, frame_ref);

  ManagedContext lalloc(vm);

  Frame* ctx_frame = charly_as_frame(frame_ref);
  if (ctx_frame == nullptr) return kNull;

  // Acquire frame information
  Frame* vm_frame      = ctx_frame->parent;
  if (vm_frame == nullptr) return kNull;

  VALUE frame_id       = charly_create_pointer(vm_frame);
  VALUE caller_value   = vm_frame->caller_value;
  VALUE self_value     = vm_frame->self;
  VALUE origin_address = charly_create_integer(reinterpret_cast<size_t>(vm_frame->origin_address));

  // Create stacktrace entry
  Object* obj = charly_as_object(lalloc.create_object(3));
  obj->container->insert({SymbolTable::encode("id"),             frame_id});
  obj->container->insert({SymbolTable::encode("caller"),         caller_value});
  obj->container->insert({SymbolTable::encode("self_value"),     self_value});
  obj->container->insert({SymbolTable::encode("origin_address"), origin_address});

  return charly_create_pointer(obj);
}

VALUE get_block_address(VM& vm, VALUE func) {
  switch (charly_get_type(func)) {
    case kTypeFunction: {
      Function* ptr = charly_as_function(func);
      return charly_create_number(reinterpret_cast<uint64_t>(ptr->body_address));
    }
    case kTypeGenerator: {
      Generator* ptr = charly_as_generator(func);
      Function* ctx_frame_func = charly_as_function(ptr->context_frame->caller_value);
      return charly_create_number(reinterpret_cast<uint64_t>(ctx_frame_func->body_address));
    }
    default: {
      vm.throw_exception("Expected function or generator");
      return kNull;
    }
  }
}

VALUE resolve_address(VM& vm, VALUE address) {
  CHECK(number, address);

  uint8_t* ip = reinterpret_cast<uint8_t*>(charly_number_to_uint64(address));
  std::optional<std::string> lookup_result = vm.context.compiler_manager.address_mapping.resolve_address(ip);

  if (!lookup_result) {
    return kNull;
  }

  return vm.create_string(lookup_result.value());
}

VALUE debug_func(VM& vm, VALUE value) {
  CHECK(string, value);

  vm.context.out_stream << "sizeof(MemoryCell) = " << sizeof(MemoryCell) << std::endl;
  vm.context.out_stream << "sizeof(Basic)      = " << sizeof(Basic) << std::endl;
  vm.context.out_stream << "sizeof(Object)     = " << sizeof(Object) << std::endl;
  vm.context.out_stream << "sizeof(Array)      = " << sizeof(Array) << std::endl;
  vm.context.out_stream << "sizeof(String)     = " << sizeof(String) << std::endl;
  vm.context.out_stream << "sizeof(Function)   = " << sizeof(Function) << std::endl;
  vm.context.out_stream << "sizeof(CFunction)  = " << sizeof(CFunction) << std::endl;
  vm.context.out_stream << "sizeof(Generator)  = " << sizeof(Generator) << std::endl;
  vm.context.out_stream << "sizeof(Class)      = " << sizeof(Class) << std::endl;
  vm.context.out_stream << "sizeof(Frame)      = " << sizeof(Frame) << std::endl;
  vm.context.out_stream << "sizeof(CatchTable) = " << sizeof(CatchTable) << std::endl;
  vm.context.out_stream << "sizeof(CPointer)   = " << sizeof(CPointer) << std::endl;

  return kNull;
}

}  // namespace Internals
}  // namespace Charly
