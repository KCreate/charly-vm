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

#include "value.h"

namespace Charly {

void CFunction::init(VALUE name, void* pointer, uint32_t argc) {
  assert(charly_is_symbol(name));

  Container::init(kTypeCFunction, 2);
  this->name              = name;
  this->pointer           = pointer;
  this->argc              = argc;
  this->push_return_value = true;
  this->halt_after_return = false;
}

void CFunction::init(CFunction* source) {
  Container::init(source);
  this->name              = source->name;
  this->pointer           = source->pointer;
  this->argc              = source->argc;
  this->push_return_value = source->push_return_value;
  this->halt_after_return = source->halt_after_return;
}

void CFunction::set_push_return_value(bool value) {
  this->push_return_value = value;
}

void CFunction::set_halt_after_return(bool value) {
  this->halt_after_return = value;
}

VALUE CFunction::get_name() {
  return this->name;
}

void* CFunction::get_pointer() {
  return this->pointer;
}

uint32_t CFunction::get_argc() {
  return this->argc;
}

bool CFunction::get_push_return_value() {
  return this->push_return_value;
}

bool CFunction::get_halt_after_return() {
  return this->halt_after_return;
}

#define VALUE_01 VALUE
#define VALUE_02 VALUE_01, VALUE
#define VALUE_03 VALUE_02, VALUE
#define VALUE_04 VALUE_03, VALUE
#define VALUE_05 VALUE_04, VALUE
#define VALUE_06 VALUE_05, VALUE
#define VALUE_07 VALUE_06, VALUE
#define VALUE_08 VALUE_07, VALUE
#define VALUE_09 VALUE_08, VALUE
#define VALUE_10 VALUE_09, VALUE
#define VALUE_11 VALUE_10, VALUE
#define VALUE_12 VALUE_11, VALUE
#define VALUE_13 VALUE_12, VALUE
#define VALUE_14 VALUE_13, VALUE
#define VALUE_15 VALUE_14, VALUE
#define VALUE_16 VALUE_15, VALUE
#define VALUE_17 VALUE_16, VALUE
#define VALUE_18 VALUE_17, VALUE
#define VALUE_19 VALUE_18, VALUE
#define VALUE_20 VALUE_19, VALUE

#define ARG_01 argv[0]
#define ARG_02 ARG_01, argv[1]
#define ARG_03 ARG_02, argv[2]
#define ARG_04 ARG_03, argv[3]
#define ARG_05 ARG_04, argv[4]
#define ARG_06 ARG_05, argv[5]
#define ARG_07 ARG_06, argv[6]
#define ARG_08 ARG_07, argv[7]
#define ARG_09 ARG_08, argv[8]
#define ARG_10 ARG_09, argv[9]
#define ARG_11 ARG_10, argv[10]
#define ARG_12 ARG_11, argv[11]
#define ARG_13 ARG_12, argv[12]
#define ARG_14 ARG_13, argv[13]
#define ARG_15 ARG_14, argv[14]
#define ARG_16 ARG_15, argv[15]
#define ARG_17 ARG_16, argv[16]
#define ARG_18 ARG_17, argv[17]
#define ARG_19 ARG_18, argv[18]
#define ARG_20 ARG_19, argv[19]

CFunction::Result CFunction::call(VM* vm_handle, uint32_t argc, VALUE* argv) {
  if (argc < this->get_argc()) return kNull;

  void* pointer = this->get_pointer();
  switch (this->get_argc()) {
    case  0: return reinterpret_cast<CFunction::Result (*)(VM&)>           (pointer)(*vm_handle);
    case  1: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_01)> (pointer)(*vm_handle, ARG_01);
    case  2: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_02)> (pointer)(*vm_handle, ARG_02);
    case  3: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_03)> (pointer)(*vm_handle, ARG_03);
    case  4: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_04)> (pointer)(*vm_handle, ARG_04);
    case  5: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_05)> (pointer)(*vm_handle, ARG_05);
    case  6: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_06)> (pointer)(*vm_handle, ARG_06);
    case  7: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_07)> (pointer)(*vm_handle, ARG_07);
    case  8: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_08)> (pointer)(*vm_handle, ARG_08);
    case  9: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_09)> (pointer)(*vm_handle, ARG_09);
    case 10: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_10)> (pointer)(*vm_handle, ARG_10);
    case 11: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_11)> (pointer)(*vm_handle, ARG_11);
    case 12: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_12)> (pointer)(*vm_handle, ARG_12);
    case 13: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_13)> (pointer)(*vm_handle, ARG_13);
    case 14: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_14)> (pointer)(*vm_handle, ARG_14);
    case 15: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_15)> (pointer)(*vm_handle, ARG_15);
    case 16: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_16)> (pointer)(*vm_handle, ARG_16);
    case 17: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_17)> (pointer)(*vm_handle, ARG_17);
    case 18: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_18)> (pointer)(*vm_handle, ARG_18);
    case 19: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_19)> (pointer)(*vm_handle, ARG_19);
    case 20: return reinterpret_cast<CFunction::Result (*)(VM&, VALUE_20)> (pointer)(*vm_handle, ARG_20);
  }

  return kNull;
}

#undef VALUE_01
#undef VALUE_02
#undef VALUE_03
#undef VALUE_04
#undef VALUE_05
#undef VALUE_06
#undef VALUE_07
#undef VALUE_08
#undef VALUE_09
#undef VALUE_10
#undef VALUE_11
#undef VALUE_12
#undef VALUE_13
#undef VALUE_14
#undef VALUE_15
#undef VALUE_16
#undef VALUE_17
#undef VALUE_18
#undef VALUE_19
#undef VALUE_20

#undef ARG_01
#undef ARG_02
#undef ARG_03
#undef ARG_04
#undef ARG_05
#undef ARG_06
#undef ARG_07
#undef ARG_08
#undef ARG_09
#undef ARG_10
#undef ARG_11
#undef ARG_12
#undef ARG_13
#undef ARG_14
#undef ARG_15
#undef ARG_16
#undef ARG_17
#undef ARG_18
#undef ARG_19
#undef ARG_20

}  // namespace Charly
