/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <array>
#include <condition_variable>
#include <filesystem>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <stack>
#include <vector>

#include "charly/handle.h"

#include "charly/utils/wait_flag.h"

#include "charly/core/compiler/compiler.h"
#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/scheduler.h"

#pragma once

namespace charly::core::runtime {

namespace fs = std::filesystem;

class Runtime {
  friend class GarbageCollector;

public:
  static int32_t run();

  Runtime();

  Heap* heap();
  Scheduler* scheduler();
  GarbageCollector* gc();

  bool wants_exit() const;

  // wait for the runtime to exit
  // returns the status code returned by the application
  int32_t join();

  // initiate runtime exit
  // only the first thread that calls this method will set the exit code
  void abort(int32_t exit_code);

  // wait for the runtime to finish initializing
  void wait_for_initialization();

  // misc. initialization methods
  void initialize_symbol_table(Thread* thread);
  void initialize_argv_tuple(Thread* thread);
  void initialize_builtin_functions(Thread* thread);
  void initialize_builtin_types(Thread* thread);
  void initialize_stdlib_paths();

  // declare a new global variable
  // returns kErrorOk on success
  // returns kErrorException if the variable already exists
  RawValue declare_global_variable(Thread* thread, SYMBOL name, bool constant, RawValue value);

  // read from global variable
  // returns kErrorNotFound if no such global variable exists
  RawValue read_global_variable(Thread* thread, SYMBOL name);

  // write to global variable
  // returns kErrorOk on success
  // returns kErrorNotFound if no such global variable exists
  // returns kErrorReadOnly if the variable is read-only
  RawValue set_global_variable(Thread* thread, SYMBOL name, RawValue value);

  // register a symbol in the global symbol table
  SYMBOL declare_symbol(Thread* thread, const char* data, size_t size);
  SYMBOL declare_symbol(Thread* thread, const std::string& string) {
    return declare_symbol(thread, string.data(), string.size());
  }

  // look up a symbol in the global symbol table
  // returns kNull if no such symbol exists
  RawValue lookup_symbol(SYMBOL symbol);

  // shape management
  ShapeId register_shape(RawShape shape);
  void register_shape(ShapeId id, RawShape shape);
  RawShape lookup_shape(ShapeId id);

  // sets the class that gets used as the parent class if no 'extends'
  // statement was present during class declaration
  bool builtin_class_is_registered(ShapeId shape_id);
  void set_builtin_class(ShapeId shape_id, RawClass klass);
  RawClass get_builtin_class(ShapeId shape_id);

  // checks wether the current thread (and the top frame's associated self value)
  // can access the private member of an instance and up to what offset
  uint32_t check_private_access_permitted(Thread* thread, RawInstance value);

  // lookup the name / path of a module
  std::optional<fs::path> resolve_module(const fs::path& module_path, const fs::path& origin_path) const;

  // import a module at a given path
  //
  // throws an exception and returns kErrorException if the file was not found
  //
  // returns the cached result if the module has been imported before and is unchanged
  // if multiple fibers import the same path, only the first will execute it
  // the other modules will wait for the first fiber to finish
  RawValue import_module_at_path(Thread* thread, const fs::path& path, bool treat_as_repl = false);

  // register a CompiledModule object with the runtime
  void register_module(Thread* thread, const ref<CompiledModule>& module);

  const fs::path& stdlib_directory() const;

  // invoke the callback with a reference to each runtime root
  void each_root(std::function<void(RawValue& value)> callback);

private:
  std::mutex m_mutex;
  utils::WaitFlag m_init_flag;
  utils::WaitFlag m_exit_flag;

  int32_t m_exit_code = 0;
  atomic<bool> m_wants_exit = false;

  std::unique_ptr<Heap> m_heap;
  std::unique_ptr<GarbageCollector> m_gc;
  std::unique_ptr<Scheduler> m_scheduler;

  std::vector<ref<CompiledModule>> m_compiled_modules;

  std::mutex m_symbols_mutex;
  std::unordered_map<SYMBOL, RawString> m_symbol_table;

  std::shared_mutex m_shapes_mutex;
  std::vector<RawValue> m_shapes;
  static constexpr size_t kBuiltinClassCount = static_cast<size_t>(ShapeId::kLastBuiltinShapeId) + 1;
  std::array<RawValue, kBuiltinClassCount> m_builtin_classes;

  struct GlobalVariable {
    RawValue value;
    bool constant;
  };
  std::shared_mutex m_globals_mutex;
  std::unordered_map<SYMBOL, GlobalVariable> m_global_variables;

  fs::path m_stdlib_directory;
  std::unordered_map<std::string, fs::path> m_builtin_libraries_paths;

  struct CachedModule {
    fs::path path;
    fs::file_time_type mtime;
    RawFuture module;
  };
  std::shared_mutex m_cached_modules_mutex;
  std::unordered_map<size_t, CachedModule> m_cached_modules;
};

}  // namespace charly::core::runtime
