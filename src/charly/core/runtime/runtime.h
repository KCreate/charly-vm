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

  RawData create_data(Thread* thread, ShapeId shape_id, size_t size);
  RawInstance create_instance(Thread* thread, ShapeId shape_id, size_t field_count, RawValue klass = kNull);
  RawInstance create_instance(Thread* thread, RawShape shape, RawValue klass = kNull);
  RawInstance create_instance(Thread* thread, RawClass klass);

  RawString create_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawString create_string(Thread* thread, const std::string& string) {
    return create_string(thread, string.data(), string.size(), crc32::hash_string(string));
  }
  RawString create_string(Thread* thread, const utils::Buffer& buf) {
    return create_string(thread, buf.data(), buf.size(), buf.hash());
  }

  template <typename... T>
  RawString create_string_from_template(Thread* thread, const char* str, const T&... args) {
    utils::Buffer buf;
    buf.write_formatted(str, args...);
    return create_string(thread, buf);
  }

  // create a new string by acquiring ownership over an existing allocation
  RawString acquire_string(Thread* thread, char* cstr, size_t size, SYMBOL hash);
  RawLargeString create_large_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string_acquire(Thread* thread, char* data, size_t size, SYMBOL hash);

  RawTuple create_tuple(Thread* thread, uint32_t count = 0);
  RawTuple create_tuple1(Thread* thread, RawValue value1);
  RawTuple create_tuple2(Thread* thread, RawValue value1, RawValue value2);
  RawTuple concat_tuple_value(Thread* thread, RawTuple left, RawValue value);

  RawValue create_class(Thread* thread,
                        SYMBOL name,
                        RawValue parent,
                        RawFunction constructor,
                        RawTuple member_props,
                        RawTuple member_funcs,
                        RawTuple static_prop_keys,
                        RawTuple static_prop_values,
                        RawTuple static_funcs,
                        uint32_t flags = 0);

  RawShape create_shape(Thread* thread, RawValue parent, RawTuple key_table);
  RawShape create_shape(Thread* thread, RawValue parent, std::initializer_list<std::tuple<std::string, uint8_t>> keys);

  RawFunction create_function(Thread* thread,
                              RawValue context,
                              SharedFunctionInfo* shared_info,
                              RawValue saved_self = kNull);
  RawBuiltinFunction create_builtin_function(Thread* thread, BuiltinFunctionType function, SYMBOL name, uint8_t argc);
  RawFiber create_fiber(Thread* thread, RawFunction function, RawValue self, RawValue arguments);
  RawFuture create_future(Thread* thread);
  RawValue create_exception(Thread* thread, RawValue value);
  RawException create_exception(Thread* thread, RawString value) {
    auto exception = create_exception(thread, RawValue::cast(value));
    DCHECK(!exception.is_error_exception());
    return RawException::cast(exception);
  }
  RawImportException create_import_exception(Thread* thread,
                                             const std::string& module_path,
                                             const ref<compiler::CompilationUnit>& unit);

  // creates a tuple containing a stack trace of the current thread
  // trim variable controls how many frames should be dropped
  // starting from the bottom
  RawTuple create_stack_trace(Thread* thread);

  RawValue await_future(Thread* thread, RawFuture future);
  RawValue await_fiber(Thread* thread, RawFiber fiber);
  RawValue resolve_future(Thread* thread, RawFuture future, RawValue result);
  RawValue reject_future(Thread* thread, RawFuture future, RawException exception);
  void future_wake_waiting_threads(Thread* thread, RawFuture future);

  // declare a new global variable
  // returns kErrorOk on success
  // returns kErrorException if the variable already exists
  RawValue declare_global_variable(Thread* thread, SYMBOL name, bool constant);

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

  // returns the klass value of a value
  RawClass lookup_class(RawValue value);

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
  // returns the cached result if the module has been imported before and is unchanged
  // if multiple fibers import the same path, only the first will execute it
  // the other modules will wait for the first fiber to finish
  RawValue import_module(Thread* thread, const fs::path& path, bool treat_as_repl = false);

  // register a CompiledModule object with the runtime
  void register_module(Thread* thread, const ref<CompiledModule>& module);

  const fs::path& stdlib_directory() const;

  // invoke the callback with a reference to each runtime root
  void each_root(std::function<void(RawValue& value)> callback);

private:
  std::mutex m_mutex;
  utils::WaitFlag m_init_flag;
  utils::WaitFlag m_exit_flag;

  int32_t m_exit_code;
  atomic<bool> m_wants_exit;

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
    bool initialized;
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
