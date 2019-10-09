// clang -g -Wall -fPIC -shared -o libraries/testlib.lib libraries/testlib.cpp -Iinclude -Ilibs -I/usr/local/opt/llvm/include/c++/v1 -lstdc++ -std=c++17

#include "value.h"
#include "vm.h"

using namespace Charly;

CharlyLibFuncList funclist = {{
  "add_1",
  "sub_1",
  "mul_10",
  "div_10"
}};

extern "C" CharlyLibFuncList* __charly_func_list() {
  return &funclist;
};

extern "C" VALUE add_1(VM& vm, VALUE v) {
  return charly_add_number(v, charly_create_number(1));
}

extern "C" VALUE sub_1(VM& vm, VALUE v) {
  return charly_sub_number(v, charly_create_number(1));
}

extern "C" VALUE mul_10(VM& vm, VALUE v) {
  return charly_mul_number(v, charly_create_number(10));
}

extern "C" VALUE div_10(VM& vm, VALUE v) {
  return charly_div_number(v, charly_create_number(10));
}
