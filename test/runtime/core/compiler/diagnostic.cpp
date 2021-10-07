/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#include <catch2/catch_all.hpp>

#include "charly/core/compiler/compiler.h"

using Catch::Matchers::Equals;

using namespace charly;
using namespace charly::core::compiler;

CATCH_TEST_CASE("formats errors") {
  utils::Buffer buffer("foo,");
  auto unit = Compiler::compile("test", buffer);

  CATCH_REQUIRE(unit->console.messages().size() == 1);

  std::stringstream out;
  unit->console.dump_all(out);

  CATCH_CHECK_THAT(out.str(), Equals(("test:1:4: error: unexpected ',' token, expected an expression\n"
                                "       1 | foo,\n")));
}

CATCH_TEST_CASE("formats messages without a location") {
  utils::Buffer buffer("");
  DiagnosticConsole console("test", buffer);

  console.info(Location{.valid=false}, "foo");
  console.warning(Location{.valid=false}, "bar");
  console.error(Location{.valid=false}, "baz");

  CATCH_REQUIRE(console.messages().size() == 3);

  std::stringstream out;
  console.dump_all(out);

  CATCH_CHECK_THAT(out.str(), Equals(("test: info: foo\n\n"
                                "test: warning: bar\n\n"
                                "test: error: baz\n")));
}

CATCH_TEST_CASE("formats multiple lines") {
  utils::Buffer buffer("\n\n(25      25)\n\n");
  auto unit = Compiler::compile("test", buffer);

  CATCH_REQUIRE(unit->console.messages().size() == 1);

  std::stringstream out;
  unit->console.dump_all(out);

  CATCH_CHECK_THAT(out.str(), Equals(("test:3:10: error: unexpected numerical constant, expected a ')' token\n"
                                "       1 | \n"
                                "       2 | \n"
                                "       3 | (25      25)\n"
                                "       4 | \n"
                                "       5 | \n")));
}
