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

#include "frame.h"

/*
 * Creates a new frame of a given type
 *
 * @param ch_frame_type type
 * @return ch_frame
 * */
ch_frame ch_frame_create(ch_frame_type type, ch_frame* prev) {
  ch_frame frame;
  frame.type = type;
  frame.prev = prev;

  if (prev) {
    frame.prev_call        = prev->type == ch_frame_type_call ? prev : prev->prev_call;
    frame.prev_exhandler   = prev->type == ch_frame_type_exhandler ? prev : prev->prev_exhandler;
    frame.prev_environment = prev->type == ch_frame_type_environment ? prev : prev->prev_environment;
    frame.prev_redirect    = prev->type == ch_frame_type_redirect ? prev : prev->prev_redirect;
  }

  frame.ref_count = 0;
  return frame;
}

/*
 * Creates a new call frame
 *
 * @param int return_address
 * @return ch_frame
 * */
ch_frame ch_frame_create_call(int return_address, ch_frame* prev) {
  ch_frame frame = ch_frame_create(ch_frame_type_call, prev);
  frame.return_address = return_address;
  return frame;
}

/*
 * Creates a new exhandler frame
 *
 * @param int exhandler
 * @return ch_frame
 * */
ch_frame ch_frame_create_exhandler(int handler_address, ch_frame* prev) {
  ch_frame frame = ch_frame_create(ch_frame_type_exhandler, prev);
  frame.handler_address = handler_address;
  return frame;
}

/*
 * Creates a new environment frame
 *
 * @param ch_buffer* environment
 * @return ch_frame
 * */
ch_frame ch_frame_create_environment(ch_buffer* environment, ch_frame* prev) {
  ch_frame frame = ch_frame_create(ch_frame_type_environment, prev);
  frame.environment = environment;
  return frame;
}

/*
 * Creates a new redirection frame
 *
 * @param ch_frame* frame
 * @return ch_frame
 * */
ch_frame ch_frame_create_redirect(ch_frame* old_frame, ch_frame* prev) {
  ch_frame frame = ch_frame_create(ch_frame_type_redirect, prev);
  frame.redirect = old_frame;
  return frame;
}

