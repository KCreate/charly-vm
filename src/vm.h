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

enum ch_frame_type {
  ch_frame_type_call,
  ch_frame_type_exhandler,
  ch_frame_type_environment,
  ch_frame_type_redirect
};

struct ch_frame {
  ch_frame_type type;
  ch_frame* prev;
  unsigned int ref_count;

  union {

    // Redirection frames
    struct {
      ch_frame* redirect;
    }

    // Environment frames
    struct {
      ch_buffer* environment;
    }

    // Call stack frames
    struct {
      int return_address;
    } call;

    // Exception handler frames
    struct {
      int handler_address;
    } exhandler;
  };
};

struct ch_vm {
  int pc;
  int sp;
};

int vm_main(int argc, char** argv);

#endif
