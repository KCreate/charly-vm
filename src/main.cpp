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

#include "charly.h"
#include "runflags.h"

using namespace std;
using namespace Charly;

extern char** environ;

int main(int argc, char** argv) {

  // Handle all arguments
  RunFlags flags(argc, argv, environ);

  cout << "show_help = " << flags.show_help << endl;
  cout << "show_version = " << flags.show_version << endl;
  cout << "show_license = " << flags.show_license << endl;
  cout << "dump_tokens = " << flags.dump_tokens << endl;
  cout << "dump_ast = " << flags.dump_ast << endl;
  cout << "skip_execution = " << flags.skip_execution << endl;

  cout << "Arguments:" << endl;
  for (auto const& arg: flags.arguments) {
    cout << arg << endl;
  }

  cout << "Flags:" << endl;
  for (auto const& flag: flags.flags) {
    cout << flag << endl;
  }

  // Initialize the VM
  VM* vm = new VM();
  vm->run();

  delete vm;
  return 0;
}
