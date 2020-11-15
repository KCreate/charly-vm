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

#include <sstream>

#include "value.h"
#include "gc.h"

namespace Charly {
namespace Internals {
namespace PrimitiveValue {

Result to_s(VM&, VALUE value) {
  std::stringstream buffer;
  charly_to_string(buffer, value);
  return charly_allocate_string(buffer.str());
}

Result copy(VM&, VALUE value) {
  switch (charly_get_type(value)) {
    case kTypeObject:    return charly_allocate<Object>(charly_as_object(value))->as_value();
    case kTypeArray:     return charly_allocate<Array>(charly_as_array(value))->as_value();
    case kTypeString:    return charly_allocate_string(charly_string_data(value), charly_string_length(value));
    case kTypeFunction:  return charly_allocate<Function>(charly_as_function(value))->as_value();
    case kTypeCFunction: return charly_allocate<CFunction>(charly_as_cfunction(value))->as_value();

    case kTypeClass:
    case kTypeFrame:
    case kTypeCatchTable:
    case kTypeCPointer: {
      return ERR("Cannot copy value of type: " + charly_get_typestring(value));
    }
    default: return value;
  }
}

}  // namespace PrimitiveValue
}  // namespace Internals
}  // namespace Charly
