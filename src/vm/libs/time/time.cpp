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

#include <chrono>
#include "vm.h"

using namespace std;

namespace Charly {
namespace Internals {
namespace Time {

using TimepointSystem = std::chrono::time_point<std::chrono::system_clock>;
using TimepointSteady = std::chrono::time_point<std::chrono::steady_clock>;

VALUE system_clock_now(VM& vm) {
  (void)vm;
  TimepointSystem now = std::chrono::system_clock::now();
  return charly_create_double(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

VALUE steady_clock_now(VM& vm) {
  (void)vm;
  TimepointSteady now = std::chrono::steady_clock::now();
  return charly_create_double(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace Time
}  // namespace Internals
}  // namespace Charly
