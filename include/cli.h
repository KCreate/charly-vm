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

#include "runflags.h"

#pragma once

namespace Charly {
static const std::string kHelpMessage =
    "Usage: charly [filename] [flags] [arguments]\n"
    "    -f FLAG, --flag FLAG             Set a flag\n"
    "    -h, --help                       Print this help message\n"
    "    -v, --version                    Prints the version number\n"
    "    -l, --license                    Prints the license\n"
    "\n"
    "Flags: \n"
    "    dump_ast                         Display the AST of the input file\n"
    "    dump_tokens                      Display tokens of the input file\n"
    "    dump_asm                         Display a disassembly of the input file\n"
    "    asm_no_offsets                   Don't display offsets in the disassembly\n"
    "    asm_no_branches                  Don't display branches as arrows in the disassembly\n"
    "    asm_no_func_branches             Don't display branches for PutFunction instructions\n"
    "    skipexec                         Don't execute after parsing\n"
    "    instruction_profile              Display a profile of all executed instructions\n"
    "    trace_opcodes                    Display opcodes as they are being executed\n"
    "    trace_catchtables                Display catchtables as they are being created or left\n"
    "    trace_frames                     Display frames as they are being entered and left\n"
    "    trace_gc                         Display statistics about the gc at runtime\n";

static const std::string kLicense =
    "MIT License \n"
    "\n"
    "Copyright (c) 2017 Leonard Schütz \n"
    "\n"
    "Permission is hereby granted, free of charge, to any person obtaining a copy \n"
    "of this software and associated documentation files (the \"Software\"), to deal \n"
    "in the Software without restriction, including without limitation the rights \n"
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"
    "copies of the Software, and to permit persons to whom the Software is \n"
    "furnished to do so, subject to the following conditions: \n"
    "The above copyright notice and this permission notice shall be included in all \n"
    "copies or substantial portions of the Software. \n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE \n"
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE \n"
    "SOFTWARE.";

static const std::string kVersion = "pre-alpha development release";

class CLI {
  RunFlags flags;

public:
  CLI(int argc, char** argv, char** environment) : flags(argc, argv, environment) {
  }
  int run();
};
}  // namespace Charly
