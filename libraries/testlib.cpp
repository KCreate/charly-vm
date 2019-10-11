// clang -g -Wall -fPIC -shared -o libraries/testlib.lib libraries/testlib.cpp -Iinclude -Ilibs -I/usr/local/opt/llvm/include/c++/v1 -lstdc++ -std=c++17

#include "value.h"
#include "vm.h"

using namespace Charly;

/* ###--- Manifest of userspace functions ---### */

CHARLY_MANIFEST(
  F(add, 1)
  F(read, 0)
)

/* ###--- Charly ---### */

int* counter;

CHARLY_API(__charly_constructor, () {
  counter = (int*)malloc(sizeof(int));
  std::cout << "allocated " << sizeof(int) << " bytes" << std::endl;
  return kNull;
})

CHARLY_API(__charly_destructor, () {
  free(counter);
  std::cout << "freed " << sizeof(int) << " bytes" << std::endl;
  return kNull;
})

/* ###--- Userspace ---### */

CHARLY_API(add, (VM& vm, VALUE v) {
  if (charly_is_number(v)) {
    *counter += charly_int_to_int32(v);
  }
  return charly_create_number(*counter);
})

CHARLY_API(read, () {
  return charly_create_number(*counter);
})
