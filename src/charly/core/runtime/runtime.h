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

#include <array>
#include <condition_variable>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <stack>
#include <vector>

#include "charly/handle.h"

#include "charly/utils/wait_flag.h"

#include "charly/core/runtime/compiled_module.h"
#include "charly/core/runtime/gc.h"
#include "charly/core/runtime/heap.h"
#include "charly/core/runtime/scheduler.h"

#pragma once

namespace charly::core::runtime {

class Runtime {
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

  // register a CompiledModule object with the runtime
  void register_module(Thread* thread, const ref<CompiledModule>& module);

  // misc. initialization methods
  void initialize_symbol_table(Thread* thread);
  void initialize_argv_tuple(Thread* thread);
  void initialize_builtin_functions(Thread* thread);
  void initialize_builtin_types(Thread* thread);
  void initialize_main_fiber(Thread* thread, SharedFunctionInfo* info);
  void initialize_global_variables(Thread* thread);

  RawData create_data(Thread* thread, ShapeId shape_id, size_t size);
  RawInstance create_instance(Thread* thread, ShapeId shape_id, size_t field_count, RawValue klass = kNull);
  RawInstance create_instance(Thread* thread, RawShape shape, RawValue klass = kNull);
  RawInstance create_instance(Thread* thread, RawClass klass);

  RawValue create_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawValue create_string(Thread* thread, const std::string& string) {
    return create_string(thread, string.data(), string.size(), crc32::hash_string(string));
  }
  RawValue create_string(Thread* thread, const utils::Buffer& buf) {
    return create_string(thread, buf.data(), buf.size(), buf.hash());
  }

  template <typename... T>
  RawValue create_string_from_template(Thread* thread, const char* str, const T&... args) {
    utils::Buffer buf;
    buf.write_formatted(str, args...);
    return create_string(thread, buf);
  }

  // create a new string by acquiring ownership over an existing allocation
  RawValue acquire_string(Thread* thread, char* cstr, size_t size, SYMBOL hash);
  RawLargeString create_large_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string(Thread* thread, const char* data, size_t size, SYMBOL hash);
  RawHugeString create_huge_string_acquire(Thread* thread, char* data, size_t size, SYMBOL hash);

  RawTuple create_tuple(Thread* thread, uint32_t count = 0);
  RawTuple create_tuple(Thread* thread, std::initializer_list<RawValue> values);
  RawTuple concat_tuple(Thread* thread, RawTuple left, RawTuple right);
  RawTuple concat_tuple_value(Thread* thread, RawTuple left, RawValue value);

  RawClass create_class(Thread* thread,
                        SYMBOL name,
                        RawClass parent,
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
  RawValue create_exception(Thread* thread, RawValue message);
  template <typename... T>
  RawValue create_exception_with_message(Thread* thread, const char* str, const T&... args) {
    return create_exception(thread, create_string_from_template(thread, str, args...));
  }

  // creates a tuple containing a stack trace of the current thread
  // trim variable controls how many frames should be dropped
  // starting from the bottom
  RawTuple create_stack_trace(Thread* thread, uint32_t trim = 0);

  RawValue join_fiber(Thread* thread, RawFiber fiber);

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
  RawShape lookup_shape(Thread* thread, ShapeId id);

  // returns the RawClass for any type
  RawClass lookup_class(Thread* thread, RawValue value);

  // sets the class that gets used as the parent class if no 'extends'
  // statement was present during class declaration
  void set_builtin_class(Thread* thread, ShapeId shape_id, RawClass klass);
  RawClass get_builtin_class(Thread* thread, ShapeId shape_id);

  // checks wether the current thread (and the top frame's associated self value)
  // can access the private member of an instance and up to what offset
  uint32_t check_private_access_permitted(Thread* thread, RawInstance value);

private:
  uint64_t m_start_timestamp;

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
};

}  // namespace charly::core::runtime
