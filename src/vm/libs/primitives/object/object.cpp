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

#include "object.h"
#include "vm.h"
#include "managedcontext.h"

namespace Charly {
namespace Internals {
namespace PrimitiveObject {

VALUE keys(VM& vm, VALUE obj) {
  ManagedContext lalloc(vm);

  // Get the container of the value
  std::unordered_map<VALUE, VALUE>* container = charly_get_container(obj);
  if (container == nullptr)
    return lalloc.create_array(0);

  // Create an array containing the keys of the container
  Array* arr = charly_as_array(lalloc.create_array(container->size()));

  for (auto& entry : *container) {
    VALUE key = lalloc.create_string(vm.context.symtable(entry.first).value_or(kUndefinedSymbolString));
    arr->data->push_back(key);
  }

  return charly_create_pointer(arr);
}

VALUE delete_key(VM& vm, VALUE v, VALUE symbol) {
  CHECK(string, symbol);

  // Check if the value has a container
  auto* container = charly_get_container(v);
  if (!container) return v;

  // Erase key from container
  VALUE key_symbol = charly_create_symbol(symbol);
  container->erase(key_symbol);

  return v;
}

}  // namespace PrimitiveObject
}  // namespace Internals
}  // namespace Charly
