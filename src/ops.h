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

#ifndef OPS_H
#define OPS_H

enum ch_op {
  ch_op_create_environment,
  ch_op_create_value,
  ch_op_write_to_environment,
  ch_op_print_environment,
  ch_op_halt,
  ch_op_debug_print,
  ch_op_num_ops
};

#define CREATE_ENVIRONMENT ch_op_create_environment
#define CREATE_VALUE ch_op_create_value
#define WRITE_TO_ENVIRONMENT ch_op_write_to_environment
#define PRINT_ENVIRONMENT ch_op_print_environment
#define HALT ch_op_halt
#define DEBUG_PRINT ch_op_debug_print
#define CH_VM_NUM_OPS ch_op_num_ops

#endif
