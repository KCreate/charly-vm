/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include "charly/handle.h"

#include "charly/core/runtime/thread.h"

namespace charly::core::runtime {

template <typename T>
Handle<T>::Handle(HandleScope& scope) : T(), m_thread(scope.thread()), m_next(m_thread->handles()->push(pointer())) {}

template <typename T>
Handle<T>::Handle(HandleScope& scope, RawValue value) :
  T(value.rawCast<T>()), m_thread(scope.thread()), m_next(m_thread->handles()->push(pointer())) {
  DCHECK(is_valid_type());
}

template <typename T>
Handle<T>::~Handle() {
  DCHECK(m_thread->handles()->head() == pointer());
  m_thread->handles()->pop(m_next);
}

#define HANDLE_DEF(T) template class Handle<Raw##T>;
TYPE_NAMES(HANDLE_DEF)
#undef HANDLE_DEF

}  // namespace charly::core::runtime
