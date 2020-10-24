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
#include <algorithm>

#include "string.h"
#include "vm.h"

namespace Charly {
namespace Internals {
namespace PrimitiveString {

VALUE to_n(VM& vm, VALUE str) {
  CHECK(string, str);
  return charly_create_number(charly_string_to_double(str));
}

VALUE ltrim(VM& vm, VALUE src) {
  CHECK(string, src);

  std::string _str(charly_string_data(src), charly_string_length(src));
  _str.erase(0, _str.find_first_not_of(" \t\n\v\f\r"));
  return vm.create_string(_str);
}

VALUE rtrim(VM& vm, VALUE src) {
  CHECK(string, src);

  std::string _str(charly_string_data(src), charly_string_length(src));
  _str.erase(_str.find_last_not_of(" \t\n\v\f\r") + 1);
  return vm.create_string(_str);
}

// TODO: Implement utf8 conversions
VALUE lowercase(VM& vm, VALUE src) {
  CHECK(string, src);
  std::string _str(charly_string_data(src), charly_string_length(src));

  std::transform(_str.begin(), _str.end(), _str.begin(), [](char c) { return std::tolower(c, std::locale()); });

  return vm.create_string(_str);
}

// TODO: Implement utf8 conversions
VALUE uppercase(VM& vm, VALUE src) {
  CHECK(string, src);
  std::string _str(charly_string_data(src), charly_string_length(src));

  std::transform(_str.begin(), _str.end(), _str.begin(), [](char c) { return std::toupper(c, std::locale()); });

  return vm.create_string(_str);
}

}  // namespace PrimitiveString
}  // namespace Internals
}  // namespace Charly
