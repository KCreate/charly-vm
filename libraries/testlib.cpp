// clang -g -Wall -fPIC -shared -o libraries/testlib.lib libraries/testlib.cpp -Iinclude -Ilibs -I/usr/local/opt/llvm/include/c++/v1 -lstdc++ -std=c++17

#include "value.h"
#include "vm.h"

using namespace Charly;

/* ###--- Library Manifesto ---### */

CHARLY_MANIFEST(
  F(add_1, 1)
  F(sub_1, 1)
  F(mul_10, 1)
  F(div_10, 1)
)

/* ###--- Library Methods ---### */

CHARLY_API(add_1, VALUE v, {
  return charly_add_number(v, charly_create_number(1));
})

CHARLY_API(sub_1, VALUE v, {
  return charly_sub_number(v, charly_create_number(1));
})

CHARLY_API(mul_10, VALUE v, {
  return charly_mul_number(v, charly_create_number(10));
})

CHARLY_API(div_10, VALUE v, {
  return charly_div_number(v, charly_create_number(10));
})
