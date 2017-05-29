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

#ifndef VM_H
#define VM_H

#include "constants.h"
#include "buffer.h"
#include "frame.h"
#include "ops.h"
#include "value.h"
#include "environment.h"

// Size constants
#define CH_VM_MEMORY_SIZE 256

// Error codes
#define CH_VM_ERR_SUCCESS 0
#define CH_VM_ERR_OUT_OF_BOUNDS 1
#define CH_VM_ERR_ILLEGAL_OP 2
#define CH_VM_ERR_ALLOCATION_FAILURE 3

struct ch_vm {
  unsigned int pc;
  unsigned int sp;

  bool running;
  bool halted;

  unsigned int errno;

  ch_frame* current_frame;
  ch_buffer memory;
};

int ch_vm_main(int argc, char** argv);

ch_vm* ch_vm_create();
ch_vm* ch_vm_init(ch_vm* vm);
void ch_vm_cycle(ch_vm* vm);
size_t ch_vm_decode_instruction_length(ch_vm* vm, char opcode);
bool ch_vm_check_out_of_bounds(ch_vm* vm, unsigned int address, size_t size);
char* ch_vm_error_string(unsigned int errno);
void ch_vm_free(ch_vm* vm);

size_t ch_vm_length_lookup_table[CH_VM_NUM_OPS];

#endif
