#include <string>
#include <iostream>
#include "value.h"

using namespace Charly;

int main() {

  // charly_create_number
  std::cout << "charly_create_number" << '\n';
  std::cout << charly_get_typestring(charly_create_number(1)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(-1)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(255)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(-255)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(25.25)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(-25.25)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(1ULL << 30)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(1ULL << 40)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(1ULL << 50)) << '\n';
  std::cout << charly_get_typestring(charly_create_number(1ULL << 60)) << '\n';
  std::cout << '\n';

  // charly_create_integer
  std::cout << "charly_create_integer" << '\n';
  std::cout << charly_get_typestring(charly_create_integer(1)) << '\n';
  std::cout << charly_get_typestring(charly_create_integer(-1)) << '\n';
  std::cout << charly_get_typestring(charly_create_integer(255)) << '\n';
  std::cout << charly_get_typestring(charly_create_integer(-255)) << '\n';
  std::cout << charly_get_typestring(charly_create_integer(250000)) << '\n';
  std::cout << '\n';

  // charly_create_double
  std::cout << "charly_create_double" << '\n';
  std::cout << charly_get_typestring(charly_create_double(1)) << '\n';
  std::cout << charly_get_typestring(charly_create_double(-1)) << '\n';
  std::cout << charly_get_typestring(charly_create_double(255)) << '\n';
  std::cout << charly_get_typestring(charly_create_double(-255)) << '\n';
  std::cout << charly_get_typestring(charly_create_double(250000)) << '\n';
  std::cout << '\n';

  // charly_create_pstring
  VALUE ps1, ps2;
  std::cout << "charly_create_pstring" << '\n';
  std::cout << charly_get_typestring(ps1 = charly_create_pstring("abcdef")) << '\n';
  std::cout << charly_get_typestring(ps2 = charly_create_pstring("fedcba")) << '\n';
  std::cout << '\n';

  // charly_string_data(charly_create_pstring)
  std::cout << "charly_string_data(charly_create_pstring)" << '\n';
  std::cout << (void*)charly_string_data(ps1) << '\n';
  std::cout << (void*)charly_string_data(ps2) << '\n';
  std::cout << '\n';

  // charly_string_length(charly_create_pstring)
  std::cout << "charly_string_length(charly_create_pstring)" << '\n';
  std::cout << charly_string_length(ps1) << '\n';
  std::cout << charly_string_length(ps2) << '\n';
  std::cout << '\n';

  // charly_create_istring
  VALUE s1, s2, s3, s4, s5, s6;
  std::cout << "charly_create_istring" << '\n';
  std::cout << charly_get_typestring(s1 = charly_create_istring("")) << '\n';
  std::cout << charly_get_typestring(s2 = charly_create_istring("1")) << '\n';
  std::cout << charly_get_typestring(s3 = charly_create_istring("12")) << '\n';
  std::cout << charly_get_typestring(s4 = charly_create_istring("123")) << '\n';
  std::cout << charly_get_typestring(s5 = charly_create_istring("1234")) << '\n';
  std::cout << charly_get_typestring(s6 = charly_create_istring("12345")) << '\n';
  std::cout << '\n';

  // charly_string_data(charly_create_istring)
  std::cout << "charly_string_data(charly_create_istring)" << '\n';
  std::cout << (void*)charly_string_data(s1) << '\n';
  std::cout << (void*)charly_string_data(s2) << '\n';
  std::cout << (void*)charly_string_data(s3) << '\n';
  std::cout << (void*)charly_string_data(s4) << '\n';
  std::cout << (void*)charly_string_data(s5) << '\n';
  std::cout << (void*)charly_string_data(s6) << '\n';
  std::cout << '\n';

  // charly_string_length(charly_create_istring)
  std::cout << "charly_string_length(charly_create_istring)" << '\n';
  std::cout << charly_string_length(s1) << '\n';
  std::cout << charly_string_length(s2) << '\n';
  std::cout << charly_string_length(s3) << '\n';
  std::cout << charly_string_length(s4) << '\n';
  std::cout << charly_string_length(s5) << '\n';
  std::cout << charly_string_length(s6) << '\n';
  std::cout << '\n';

  // singletons
  std::cout << "singletons" << '\n';
  std::cout << charly_get_typestring(kFalse) << '\n';
  std::cout << charly_get_typestring(kTrue) << '\n';
  std::cout << charly_get_typestring(kNull) << '\n';
  std::cout << charly_get_typestring(kNaN) << '\n';
  std::cout << '\n';

  return 0;
}
