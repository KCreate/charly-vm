/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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

#pragma once

namespace charly::utils {

// Source: https://stackoverflow.com/a/11763277/2611393
#define CHARLY_FE_1(WHAT, X) WHAT(X)
#define CHARLY_FE_2(WHAT, X, ...) WHAT(X) CHARLY_FE_1(WHAT, __VA_ARGS__)
#define CHARLY_FE_3(WHAT, X, ...) WHAT(X) CHARLY_FE_2(WHAT, __VA_ARGS__)
#define CHARLY_FE_4(WHAT, X, ...) WHAT(X) CHARLY_FE_3(WHAT, __VA_ARGS__)
#define CHARLY_FE_5(WHAT, X, ...) WHAT(X) CHARLY_FE_4(WHAT, __VA_ARGS__)

#define CHARLY_GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME
#define CHARLY_VA_FOR_EACH(action, ...) \
  CHARLY_GET_MACRO(__VA_ARGS__, CHARLY_FE_5, CHARLY_FE_4, CHARLY_FE_3, CHARLY_FE_2, CHARLY_FE_1)(action, __VA_ARGS__)

#define CHARLY_NON_CONSTRUCTABLE(C) \
private:                            \
  C() = delete;                     \
                                    \
private:                            \
  ~C() = delete;

#define CHARLY_NON_COPYABLE(C)     \
private:                           \
  C(const C&) = delete;            \
                                   \
private:                           \
  C(C&) = delete;                  \
                                   \
private:                           \
  C(const C&&) = delete;           \
                                   \
private:                           \
  C(C&&) = delete;                 \
                                   \
private:                           \
  C& operator=(C&) = delete;       \
                                   \
private:                           \
  C& operator=(C&&) = delete;      \
                                   \
private:                           \
  C& operator=(const C&) = delete; \
                                   \
private:                           \
  C& operator=(const C&&) = delete;

#define UNREACHABLE()                          \
  assert(false && "reached unreachable code"); \
  __builtin_unreachable();

}  // namespace charly::utils
