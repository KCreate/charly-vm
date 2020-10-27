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

namespace Charly {
namespace Internals {
namespace PrimitiveObject {

VALUE keys(VM& vm, VALUE obj) {
  CHECK(container, obj);

  Immortal<Array> keys_array = vm.gc.allocate<Array>(4);

  Container* obj_container = charly_as_container(obj);
  obj_container->access_container_shared([&](Container::ContainerType* container) {
    for (auto& [key, value] : *container) {
      keys_array->push(vm.gc.allocate<String>(SymbolTable::decode(key))->as_value());
    }
  });

  return keys_array->as_value();
}

VALUE delete_key(VM& vm, VALUE v, VALUE symbol) {
  CHECK(string, symbol);
  CHECK(container, v);

  charly_as_container(v)->erase(charly_create_symbol(symbol));

  return v;
}

}  // namespace PrimitiveObject
}  // namespace Internals
}  // namespace Charly
