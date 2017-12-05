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

#include "charly.h"
#include "cli.h"
#include "context.h"
#include "sourcefile.h"

namespace Charly {
int CLI::run() {
  if (this->flags.show_help) {
    std::cout << kHelpMessage << std::endl;
    return 0;
  }

  if (this->flags.show_license) {
    std::cout << kLicense << std::endl;
    return 0;
  }

  if (this->flags.show_version) {
    std::cout << kVersion << std::endl;
    return 0;
  }

  // Check if a filename was given
  if (this->flags.arguments.size() == 0) {
    std::cout << "No filename given!" << std::endl;
    return 1;
  }

  std::ifstream inputfile(this->flags.arguments[0]);

  if (!inputfile.is_open()) {
    std::cout << "Could not open file" << std::endl;
    return 1;
  }

  std::string source_string((std::istreambuf_iterator<char>(inputfile)), std::istreambuf_iterator<char>());

  inputfile.close();

  SourceFile userfile(this->flags.arguments[0], source_string);

  Context context(this->flags);
  MemoryManager gc(context);
  VM vm(context);
  if (!this->flags.skip_execution) {
    vm.run();
  }

  return 0;
}
}  // namespace Charly
