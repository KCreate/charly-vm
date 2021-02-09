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

#include <list>

#include "charly/utils/cast.h"
#include "charly/core/compiler/passes/desugar_pass.h"

namespace charly::core::compiler::ast {

ref<Expression> DesugarPass::transform(const ref<FormatString>& node) {
  ref<BuiltinOperation> builtin = make<BuiltinOperation>(make<Name>("stringconcat"));

  for (const ref<Expression>& element : node->elements) {
    if (isa<String>(element)) {
      builtin->arguments.push_back(element);
      continue;
    }

    builtin->arguments.push_back(make<BuiltinOperation>(make<Name>("caststring"), element));
  }

  if (builtin->arguments.size() == 1) {
    return builtin->arguments.back();
  }

  return builtin;
}

}  // namespace charly::core::compiler::ast
