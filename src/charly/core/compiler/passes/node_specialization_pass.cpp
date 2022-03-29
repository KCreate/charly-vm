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

#include "charly/core/compiler/ir/ir.h"
#include "charly/core/compiler/passes/node_specialization_pass.h"
#include "charly/utils/cast.h"

namespace charly::core::compiler::ast {

using namespace charly::core::compiler::ir;

ref<Expression> NodeSpecializationPass::transform(const ref<Id>& node) {
  if (node->ir_location.type == ValueLocation::Type::Self) {
    return make<MemberOp>(make<Self>(), make<Name>(node));
  } else if (node->ir_location.type == ValueLocation::Type::FarSelf) {
    return make<MemberOp>(make<FarSelf>(node->ir_location.as.far_self.depth), make<Name>(node));
  }

  CHECK(node->ir_location.type != ValueLocation::Type::Self && node->ir_location.type != ValueLocation::Type::FarSelf);

  return node;
}

}  // namespace charly::core::compiler::ast
