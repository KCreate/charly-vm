/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <iostream>

#include "internals.h"
#include "gc.h"
#include "vm.h"

using namespace std;

namespace Charly {
  namespace Internals {
    VALUE get_method(VM* vm, VALUE method_name) {
      cout << "called get_method with "; vm->pretty_print(cout, method_name); cout << endl;

      VALUE obj1 = vm->create_object(0);
      vm->gc.register_temporary(obj1);

      VALUE obj2 = vm->create_object(0);
      vm->gc.register_temporary(obj2);

      VALUE obj3 = vm->create_object(0);
      vm->gc.register_temporary(obj3);

      VALUE obj4 = vm->create_object(0);
      vm->gc.register_temporary(obj4);

      vm->gc.unregister_temporary(obj1);
      vm->gc.unregister_temporary(obj2);
      vm->gc.unregister_temporary(obj3);
      vm->gc.unregister_temporary(obj4);

      return vm->create_integer(25);
    }
  }
}
