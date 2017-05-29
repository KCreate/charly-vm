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

#define VMERR(type) vm->errno = type; vm->halted = true; vm->running = false;

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
  vm->sp = CH_VM_MEMORY_SIZE - 1;
  vm->running = true;
  vm->halted = false;
  vm->errno = CH_VM_ERR_SUCCESS;
  vm->current_frame = NULL;

  // Initialize buffers
  if (!ch_buffer_init(&vm->memory, sizeof(char), CH_VM_MEMORY_SIZE)) return NULL;

  return vm;
}

/*
 * Runs a single CPU cycle in the machine
 *
 * @param ch_vm* vm
 * */
void ch_vm_cycle(ch_vm* vm) {

  // Don't run a cycle if the machine isn't running or was halted
  if (vm->running && !vm->halted) {

    // Check if the program counter is out of bounds
    if (ch_vm_check_out_of_bounds(vm, vm->pc, sizeof(char))) {
      VMERR(CH_VM_ERR_OUT_OF_BOUNDS);
      return;
    }

    // Read an opcode
    unsigned int old_pc = vm->pc;
    char opcode = vm->memory.buffer[vm->pc];

    // Check if this is a valid opcode
    if (opcode >= CH_VM_NUM_OPS) {
      VMERR(CH_VM_ERR_ILLEGAL_OP);
      return;
    }

    // Decode the instruction length and check if it's all in-bounds
    size_t instruction_length = ch_vm_decode_instruction_length(vm, opcode);
    if (ch_vm_check_out_of_bounds(vm, vm->pc, instruction_length)) {
      VMERR(CH_VM_ERR_OUT_OF_BOUNDS);
      return;
    }

    // Execute the opcode
    switch (opcode) {
      case CREATE_ENVIRONMENT: {
        unsigned int value_count = *(unsigned int *)(ch_buffer_index_ptr(&vm->memory, vm->pc + 1));

        // Allocate the environment
        ch_environment* environment = ch_environment_create(value_count);
        if (!environment) {
          VMERR(CH_VM_ERR_OUT_OF_BOUNDS);
          return;
        }

        // Allocate, initialize and update the frame
        ch_frame* frame = malloc(sizeof(ch_frame));
        if (!frame) {
          free(environment);
          VMERR(CH_VM_ERR_ALLOCATION_FAILURE);
          return;
        }

        *frame = ch_frame_create_environment(environment, vm->current_frame);
        vm->current_frame = frame;
        break;
      }

      case CREATE_VALUE: {
        printf("creating value\n");
        break;
      }

      case WRITE_TO_ENVIRONMENT: {
        printf("writing to environment\n");
        break;
      }

      case PRINT_ENVIRONMENT: {
        printf("printing environment\n");
        break;
      }

      case DEBUG_PRINT: {
        printf("Debug info:\n");

        printf("pc: 0x%08x\n", vm->pc);
        printf("sp: 0x%08x\n", vm->sp);

        printf("\n");
        printf("Frame list:\n");

        ch_frame* frame = vm->current_frame;
        int frame_i = 0;
        while (frame) {
          switch (frame->type) {
            case ch_frame_type_call:
              printf("(Call - %08x - %p)\n", frame->return_address, frame);
              break;
            case ch_frame_type_exhandler:
              printf("(ExceptionHandler - %08x - %p)\n", frame->handler_address, frame);
              break;
            case ch_frame_type_environment:
              printf("(Environment - %zu values - %p)\n", frame->environment->size, frame);
              break;
            case ch_frame_type_redirect:
              printf("(Redirect - %p - %p)\n", frame->redirect, frame);
              break;
          }

          frame = frame->prev;
          frame_i++;
        }

        printf("\n");

        break;
      }

      case HALT: {
        vm->halted = true;
        vm->running = false;
        return;
      }
    }

    // If the program counter wasn't moved by any instruction, update it now
    if (vm->pc == old_pc) {
      vm->pc += instruction_length;
    }

    return;
  }
}

/*
 * Decodes the length of the current instruction
 *
 * @param ch_vm* vm
 * @param char opcode
 * @return size_t
 * */
size_t ch_vm_decode_instruction_length(ch_vm* vm, char opcode) {

  // Check special cases
  switch (opcode) {
    case CREATE_VALUE: {

      // Check the type argument
      char type = vm->memory.buffer[vm->pc + 1];

      // Different types have different lengths
      //
      //     +- Opcode
      //     |   +- Type identifier
      //     |   |   +- Immediate value of variable size
      //     1 + 1 + x
      switch (type) {

        // Float (double-precision) values
        case ch_type_numeric: {
          return 1 + 1 + 8;
        }

        // Buffers, strings consist of a single pointer and an associated
        // size. Both parts are 4 bytes
        //
        // FF FF FF FF FF FF FF FF
        // +---------- +----------
        // |           |
        // |           +- Size of the buffer, string
        // |
        // +- Pointer to global memory
        case ch_type_buffer:
        case ch_type_string: {
          return 1 + 1 + 4 + 4;
        }

        // Functions and value pointers consist of 4 bytes
        case ch_type_environment:
        case ch_type_function:
        case ch_type_pointer: {
          return 1 + 1 + 4;
        }

        // Void pointer take up 8 bytes
        case ch_type_voidptr: {
          return 1 + 1 + 8;
        }
      }
    }
  }

  // Make sure this is a valid opcode, so we don't do an out of range table access
  if (opcode >= CH_VM_NUM_OPS) {
    return 0;
  }

  return ch_vm_length_lookup_table[opcode];
}

/*
 * Check if a given address and size is out of bounds
 *
 * @param ch_vm* vm
 * @param int address
 * @param size_t size
 * @return bool
 * */
bool ch_vm_check_out_of_bounds(ch_vm* vm, unsigned int address, size_t size) {
  if (address >= vm->memory.size) return true;
  if (address + size - 1 >= vm->memory.size) return true;
  return false;
}

/*
 * Returns a human-readable error string for a given error number
 *
 * @param unsigned int errno
 * @return char*
 * */
char* ch_vm_error_string(unsigned int errno) {
  switch (errno) {
    case CH_VM_ERR_SUCCESS:
      return "Success";
    case CH_VM_ERR_OUT_OF_BOUNDS:
      return "Memory access out of bounds";
    case CH_VM_ERR_ILLEGAL_OP:
      return "Illegal opcode";
    case CH_VM_ERR_ALLOCATION_FAILURE:
      return "Allocation failure";
    default:
      return "Unknown error";
  }
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

// Contains the length of any fixed-length instructions
size_t ch_vm_length_lookup_table[] = {
  5,      // create_environment
  0,      // create_value (calculated in ch_vm_decode_instruction_length)
  9,      // write_to_environment
  9,      // print_environment
  1,      // halt
  1       // debug_print
};

char debug_program[] = {

  CREATE_ENVIRONMENT, 0x04, 0x00, 0x00, 0x00,
  CREATE_ENVIRONMENT, 0x04, 0x00, 0x00, 0x00,
  CREATE_VALUE, ch_type_numeric, 0x40, 0x39, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
  WRITE_TO_ENVIRONMENT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  PRINT_ENVIRONMENT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  DEBUG_PRINT,
  HALT

};

int ch_vm_main(int argc, char** argv) {

  // Allocate and create the debug program
  ch_buffer* program = ch_buffer_create(sizeof(char), sizeof(debug_program));
  ch_buffer_copy_from(program, debug_program, sizeof(debug_program));

  // Allocate a vm and copy the program into it
  ch_vm* vm = ch_vm_create();
  ch_buffer_move(&vm->memory, program);

  // Run some vm cycles
  while (vm->running) {
    ch_vm_cycle(vm);
  }

  // Free the vm and program buffers
  ch_buffer_free(program);
  ch_vm_free(vm);
  free(program);
  free(vm);

  return 0;
}
