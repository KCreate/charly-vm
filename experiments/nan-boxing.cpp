#include <string>
#include <iostream>
#include "value.h"

using namespace Charly;

int main() {
  std::cout << std::hex;

  // charly_create_number
  std::cout << "charly_create_number" << '\n';
  std::cout << charly_create_number(1) << '\n';
  std::cout << charly_create_number(-1) << '\n';
  std::cout << charly_create_number(255) << '\n';
  std::cout << charly_create_number(-255) << '\n';
  std::cout << charly_create_number(25.25) << '\n';
  std::cout << charly_create_number(-25.25) << '\n';
  std::cout << charly_create_number(1ULL << 30) << '\n';
  std::cout << charly_create_number(1ULL << 31) << '\n';
  std::cout << charly_create_number(1ULL << 50) << '\n';
  std::cout << charly_create_number(1ULL << 60) << '\n';
  std::cout << '\n';

  // charly_number_to_int64
  std::cout << "charly_number_to_int64" << '\n';
  std::cout << charly_number_to_int64(charly_create_number(1)) << '\n';
  std::cout << charly_number_to_int64(charly_create_number(-1)) << '\n';
  std::cout << charly_number_to_int64(charly_create_number(255)) << '\n';
  std::cout << charly_number_to_int64(charly_create_number(-255)) << '\n';
  std::cout << charly_number_to_double(charly_create_number(25.25)) << '\n';
  std::cout << charly_number_to_double(charly_create_number(-25.25)) << '\n';
  std::cout << charly_number_to_int64(charly_create_number(1ULL << 30)) << '\n';
  std::cout << charly_number_to_int64(charly_create_number(1ULL << 31)) << '\n';
  std::cout << charly_number_to_double(charly_create_number(1ULL << 50)) << '\n';
  std::cout << charly_number_to_double(charly_create_number(1ULL << 60)) << '\n';
  std::cout << '\n';

  // charly_create_integer
  std::cout << "charly_create_integer" << '\n';
  std::cout << charly_create_integer(1) << '\n';
  std::cout << charly_create_integer(-1) << '\n';
  std::cout << charly_create_integer(255) << '\n';
  std::cout << charly_create_integer(-255) << '\n';
  std::cout << charly_create_integer(250000) << '\n';
  std::cout << '\n';

  // charly_int_to_int64
  std::cout << "charly_int_to_int64" << '\n';
  std::cout << charly_int_to_int64(charly_create_integer(1)) << '\n';
  std::cout << charly_int_to_int64(charly_create_integer(-1)) << '\n';
  std::cout << charly_int_to_int64(charly_create_integer(255)) << '\n';
  std::cout << charly_int_to_int64(charly_create_integer(-255)) << '\n';
  std::cout << charly_int_to_int64(charly_create_integer(250000)) << '\n';
  std::cout << '\n';

  // charly_create_double
  std::cout << "charly_create_double" << '\n';
  std::cout << charly_create_double(1) << '\n';
  std::cout << charly_create_double(-1) << '\n';
  std::cout << charly_create_double(255) << '\n';
  std::cout << charly_create_double(-255) << '\n';
  std::cout << charly_create_double(250000) << '\n';
  std::cout << '\n';

  // charly_double_to_int64
  std::cout << "charly_double_to_int64" << '\n';
  std::cout << charly_double_to_int64(charly_create_double(1)) << '\n';
  std::cout << charly_double_to_int64(charly_create_double(-1)) << '\n';
  std::cout << charly_double_to_int64(charly_create_double(255)) << '\n';
  std::cout << charly_double_to_int64(charly_create_double(-255)) << '\n';
  std::cout << charly_double_to_int64(charly_create_double(250000)) << '\n';
  std::cout << '\n';

  // charly_create_istring
  VALUE ps1, ps2;
  std::cout << "charly_create_istring" << '\n';
  std::cout << (ps1 = charly_create_istring("abcdef")) << '\n';
  std::cout << (ps2 = charly_create_istring("fedcba")) << '\n';
  std::cout << '\n';

  // charly_string_data(charly_create_istring)
  std::cout << "charly_string_data(charly_create_istring)" << '\n';
  std::cout << (void*)charly_string_data(ps1) << '\n';
  std::cout << (void*)charly_string_data(ps2) << '\n';
  std::cout << '\n';

  // charly_string_length(charly_create_istring)
  std::cout << "charly_string_length(charly_create_istring)" << '\n';
  std::cout << charly_string_length(ps1) << '\n';
  std::cout << charly_string_length(ps2) << '\n';
  std::cout << '\n';

  // dumping pstring
  std::cout << "dumping istring" << '\n';
  std::cout.write(charly_string_data(ps1), charly_string_length(ps1)) << '\n';
  std::cout.write(charly_string_data(ps2), charly_string_length(ps2)) << '\n';
  std::cout << '\n';

  // charly_create_istring
  VALUE s1, s2, s3, s4, s5, s6;
  std::cout << "charly_create_istring" << '\n';
  std::cout << (s1 = charly_create_istring("")) << '\n';
  std::cout << (s2 = charly_create_istring("1")) << '\n';
  std::cout << (s3 = charly_create_istring("12")) << '\n';
  std::cout << (s4 = charly_create_istring("123")) << '\n';
  std::cout << (s5 = charly_create_istring("1234")) << '\n';
  std::cout << (s6 = charly_create_istring("12345")) << '\n';
  std::cout << '\n';

  // charly_string_data(charly_create_istring)
  std::cout << "charly_string_data(charly_create_istring)" << '\n';
  std::cout << s1 << ": " << (void*)charly_string_data(s1) << '\n';
  std::cout << s2 << ": " << (void*)charly_string_data(s2) << '\n';
  std::cout << s3 << ": " << (void*)charly_string_data(s3) << '\n';
  std::cout << s4 << ": " << (void*)charly_string_data(s4) << '\n';
  std::cout << s5 << ": " << (void*)charly_string_data(s5) << '\n';
  std::cout << s6 << ": " << (void*)charly_string_data(s6) << '\n';
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

  // dumping istring
  std::cout << "dumping istring" << '\n';
  std::cout.write(charly_string_data(s1), charly_string_length(s1)) << '\n';
  std::cout.write(charly_string_data(s2), charly_string_length(s2)) << '\n';
  std::cout.write(charly_string_data(s3), charly_string_length(s3)) << '\n';
  std::cout.write(charly_string_data(s4), charly_string_length(s4)) << '\n';
  std::cout.write(charly_string_data(s5), charly_string_length(s5)) << '\n';
  std::cout.write(charly_string_data(s6), charly_string_length(s6)) << '\n';
  std::cout << '\n';

  // singletons
  std::cout << "singletons" << '\n';
  std::cout << kFalse << '\n';
  std::cout << kTrue << '\n';
  std::cout << kNull << '\n';
  std::cout << kNaN << '\n';
  std::cout << charly_create_symbol("hello world") << '\n';
  std::cout << charly_create_symbol("hello world") << '\n';
  std::cout << charly_create_symbol("this is a symbol") << '\n';
  std::cout << '\n';

  // pointers
  int a = 25;
  char* out_of_bounds = reinterpret_cast<char*>(static_cast<int64_t>(1) << 50);
  std::cout << "pointers" << '\n';
  std::cout << charly_as_pointer_to<int64_t>(charly_create_pointer(nullptr)) << '\n';
  std::cout << charly_as_pointer_to<int64_t>(charly_create_pointer(&a)) << '\n';
  std::cout << charly_as_pointer_to<int64_t>(charly_create_pointer(out_of_bounds)) << '\n';
  std::cout << '\n';

  // misc. cases
  std::cout << "misc. cases" << '\n';
  // For some reason, 0.0 / 0.0 generates a signed NaN value
  std::cout << charly_create_double(0.0 / 0.0) << '\n';
  std::cout << charly_create_double(kNaN) << '\n';
  std::cout << '\n';

  return 0;
}
