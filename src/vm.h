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

#include <stack>

#include "defines.h"
#include "gc.h"
#include "value.h"
#include "scope.h"
#include "frame.h"
#include "opcode.h"
#include "block.h"

#pragma once

namespace Charly {
  namespace Machine {
    using namespace Primitive;

    class VM {
      private:
        GC::Collector* gc;
        std::vector<InstructionBlock*> unassigned_blocks;
        std::vector<VALUE> stack;
        std::unordered_map<VALUE, std::string> symbol_table;
        Frame* frames;
        uint8_t* ip;
        bool halted;

      public:

        // Methods that operate on the VM's frames
        Frame* pop_frame();
        inline Frame* top_frame() { return this->frames; }
        Frame* push_frame(VALUE self, Function* calling_function, uint8_t* return_address);

        // Read and write from/to the frame hierarchy
        STATUS read(VALUE key, VALUE* result);
        STATUS write(VALUE key, VALUE value);

        // Instruction Blocks
        InstructionBlock* request_instruction_block(uint32_t lvarcount);
        void register_instruction_block(InstructionBlock* block);
        void claim_instruction_block(InstructionBlock* block);

        // Stack manipulation
        VALUE pop_stack();
        VALUE peek_stack();
        void push_stack(VALUE value);

        // Symbol table
        std::string lookup_symbol(VALUE symbol);

        // Methods to create new data types
        VALUE create_symbol(std::string value);
        VALUE create_object(uint32_t initial_capacity, VALUE klass);
        VALUE create_integer(int64_t value);
        VALUE create_float(double value);
        VALUE create_function(VALUE name, uint32_t required_arguments, InstructionBlock* block);
        VALUE create_cfunction(VALUE name, uint32_t required_arguments, void* pointer);

        // Methods that operate on the VALUE type
        int64_t integer_value(VALUE value);
        double float_value(VALUE value);
        bool boolean_value(VALUE value);
        VALUE type(VALUE value);

        // Execution
        Opcode fetch_instruction();
        uint32_t decode_instruction_length(Opcode opcode);
        void op_readlocal(uint32_t index);
        void op_readsymbol(VALUE symbol);
        void op_readmembersymbol(VALUE symbol);
        void op_setlocal(uint32_t index);
        void op_setsymbol(VALUE symbol);
        void op_setmembersymbol(VALUE symbol);
        void op_putself();
        void op_putvalue(VALUE value);
        void op_putfunction(VALUE symbol, InstructionBlock* block, bool anonymous, uint32_t argc);
        void op_putcfunction(VALUE symbol, void* pointer, uint32_t argc);
        void call_function(Function* function, uint32_t argc, VALUE* argv);
        void call_cfunction(CFunction* function, uint32_t argc, VALUE* argv);
        void op_puthash(uint32_t size);
        void op_registerlocal(VALUE symbol, uint32_t offset);
        void op_makeconstant(uint32_t offset);
        void op_call(uint32_t argc);
        void op_throw(ThrowType type);

        // TODO: Handle other types in these methods as well
        void op_add();

      private:
        void inline panic(STATUS reason) {
          std::cout << "Panic: " << Status::str[reason] << std::endl;
          exit(1);
        }
        void stacktrace(std::ostream& io);
        void stackdump(std::ostream& io);

      public:
        VM() : gc(new GC::Collector(InitialHeapCount, HeapCellCount)) {
          this->init();
        }
        void init();
        void run();
    };
  }
}
