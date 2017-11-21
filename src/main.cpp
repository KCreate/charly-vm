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

using namespace std;
using namespace Charly;

int main() {

  // Utf8 Buffer testing
  Buffer test_buffer;

  std::string message = "öäü";
  test_buffer.write(message);

  uint8_t data[] = { 0x48, 0x61, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x65, 0x6C, 0x74 };
  test_buffer.write(data, sizeof(data));

  test_buffer.read(test_buffer);

  size_t buffersize = test_buffer.size();

  std::string buffer_content(test_buffer.buffer, test_buffer.used_bytesize);

  std::cout << "Buffer content: " << buffer_content << std::endl;
  std::cout << "Buffersize: " << buffersize << std::endl;

  // Initialize the VM
  MemoryManager gc;
  VM* vm = new VM(&gc);
  vm->run();

  delete vm;
  return 0;
}
