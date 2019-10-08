// clang -g -Wall -fPIC -shared -o libraries/testlib.lib libraries/testlib.cpp -Iinclude -Ilibs -I/usr/local/opt/llvm/include/c++/v1 -lstdc++ -std=c++17

#include "value.h"
#include "vm.h"

using namespace Charly;

CharlyLibFuncList funclist = {{"add", "sub"}};

extern "C" CharlyLibFuncList* __charly_func_list() {
  return &funclist;
};

extern "C" VALUE add(VM& vm, VALUE left, VALUE right) {
  return charly_add_number(left, right);
}

extern "C" VALUE sub(VM& vm, VALUE left, VALUE right) {
  return charly_sub_number(left, right);
}
