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

#include "vm.h"

/*
 * Allocate and initialize a new vm
 *
 * @return ch_vm*
 * */
ch_vm* ch_vm_create() {

  // Allocate space for the vm
  ch_vm* vm = malloc(sizeof(ch_vm));
  if (!vm) return NULL;

  // Initialize
  if (!ch_vm_init(vm)) {
    free(vm);
    return NULL;
  }

  return vm;
}

/*
 * Initialize a vm
 *
 * @param ch_vm* vm
 * @return ch_vm* - The argument
 * */
ch_vm* ch_vm_init(ch_vm* vm) {
  vm->pc = 0;
  vm->sp = VM_MEMORY_SIZE - 1;
  vm->current_frame = NULL;

  // Initialize buffers
  if (!ch_buffer_init(&vm->memory, sizeof(char), VM_MEMORY_SIZE)) return NULL;

  return vm;
}

/*
 * Deallocate a vm
 *
 * @param ch_vm* vm
 * */
void ch_vm_free(ch_vm* vm) {
  ch_buffer_free(&vm->memory);

  while (vm->current_frame) {
    free(vm->current_frame);
    vm->current_frame = vm->current_frame->prev;
  }
}

char debug_program[] = {

  CREATE_ENVIRONMENT, 0x04,
  CREATE_VALUE, ch_type_numeric, 0x5,
  WRITE_TO_ENVIRONMENT, 0, 0

};

int ch_vm_main(int argc, char** argv) {
  ch_buffer* program = ch_buffer_create(sizeof(char), sizeof(debug_program));
  ch_buffer_copy_from(program, debug_program, sizeof(debug_program));

  ch_vm* vm = ch_vm_create();
  ch_buffer_move(&vm->memory, program);

  ch_buffer_free(program);
  ch_vm_free(vm);

  free(program);
  free(vm);

  return 0;
}
