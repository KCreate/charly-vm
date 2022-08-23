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

#include <list>

#include "charly/core/compiler/passes/desugar_pass.h"
#include "charly/utils/cast.h"

namespace charly::core::compiler::ast {

void DesugarPass::inspect_leave(const ref<Import>& node) {
  ref<Expression> source = node->source;

  if (!isa<String>(source)) {
    source = make<BuiltinOperation>(ir::BuiltinId::caststring, source);
  }

  node->source = source;
}

bool DesugarPass::inspect_enter(const ref<Spawn>& node) {
  // spawn { <body> } ->  spawn ->{ <body> }()
  if (auto block = cast<Block>(node->statement)) {
    auto func = make<Function>(true, make<Name>("anonymous"), block);
    func->set_location(node);

    auto call = make<CallOp>(func);
    call->set_location(node);

    node->statement = call;
  } else if (auto exp = cast<Expression>(node->statement)) {
    if (!isa<CallOp>(exp)) {
      auto func_block = make<Block>(make<Return>(exp));
      auto func = make<Function>(true, make<Name>("anonymous"), func_block);
      func->set_location(node);

      ref<CallOp> call = make<CallOp>(func);
      call->set_location(node);

      node->statement = call;
    }
  }

  return true;
}

ref<Expression> DesugarPass::transform(const ref<MemberOp>& node) {
  if (node->member->value == "klass") {
    auto op = make<Typeof>(node->target);
    op->set_location(node);
    return op;
  }

  return node;
}

ref<Expression> DesugarPass::transform(const ref<IndexOp>& node) {
  if (auto string = cast<String>(node->index)) {
    auto member_op = make<MemberOp>(node->target, string->value);
    member_op->set_location(node);
    return member_op;
  }

  return node;
}

ref<Expression> DesugarPass::transform(const ref<FormatString>& node) {
  // format strings with only one element can be replaced with
  // a single caststring operation
  if (node->elements.size() == 1) {
    return make<BuiltinOperation>(ir::BuiltinId::caststring, node->elements.back());
  }

  return node;
}

bool DesugarPass::inspect_enter(const ref<Function>& node) {
  // implicitly return last expression or func/class declaration inside the function body
  DCHECK(isa<Block>(node->body));
  auto body = cast<Block>(node->body);

  if (body->statements.size() >= 1 && !node->class_constructor) {
    auto last_statement = body->statements.back();
    if (auto exp = cast<Expression>(last_statement)) {
      body->statements.back() = make<Return>(exp);
    } else if (auto decl = cast<Declaration>(last_statement)) {
      if (decl->implicit) {
        auto id = decl->name;
        body->statements.push_back(make<Return>(make<Id>(id)));
      }
    }
  }

  return true;
}

void DesugarPass::inspect_leave(const ref<Function>& node) {
  // emit self initializations of function arguments
  for (auto it = node->arguments.rbegin(); it != node->arguments.rend(); it++) {
    const ref<FunctionArgument>& arg = *it;
    if (arg->self_initializer) {
      DCHECK(node->class_constructor || node->class_member_function);
      auto assignment = make<Assignment>(make<MemberOp>(make<Self>(), arg->name), make<Id>(arg->name));
      assignment->set_location(arg);
      node->body->statements.insert(node->body->statements.begin(), assignment);
    }
  }

  // wrap regular functions with yield expressions inside a generator wrapper function
  // TODO: revisit this once figured out how yield should behave
  if (!node->arrow_function && false) {
    // check if this function contains any yield statements
    ref<Node> yield_node = Node::search(
      node->body,
      [&](const ref<Node>& node) {
        return node->type() == Node::Type::Yield;
      },
      [&](const ref<Node>& node) {
        switch (node->type()) {
          case Node::Type::Function:
          case Node::Type::Class:
          case Node::Type::Spawn: {
            return true;
          }
          default: {
            return false;
          }
        }
      });

    // transform this regular function into a generator function
    // by wrapping its original body with a return spawn statement
    // and making sure all the function arguments are passed on
    //
    // func foo(a = 1, b = 2, ...rest) {
    //   yield 1
    //   yield a
    //   yield rest
    // }
    //
    // transformed to:
    //
    // func foo(a = 1, b = 2, ...rest) {
    //   return castiterator(spawn ->(a, b, rest) {
    //     yield 1
    //     yield a
    //     yield rest
    //   }(a, b, rest))
    // }
    if (yield_node) {
      // wrapper arrow func
      ref<Function> func = make<Function>(true, make<Name>("generator_" + node->name->value), node->body);

      // insert arguments
      for (const ref<FunctionArgument>& argument : node->arguments) {
        func->arguments.push_back(make<FunctionArgument>(argument->name));
      }

      // build arrow func immediate call
      ref<CallOp> func_call = make<CallOp>(func);
      for (const ref<FunctionArgument>& argument : node->arguments) {
        func_call->arguments.push_back(make<Id>(argument->name));
      }

      // build wrapped spawn statement
      ref<Block> new_body = make<Block>(make<Return>(make<Spawn>(func_call)));
      node->body = cast<Block>(apply(new_body));
    }
  }
}

// generate default constructors for classes
//
// class A {
//   property a = 1
//   property b = 2
//   property c = 3
//
//   constructor(@a, @b, @c)
// }
//
// note: the following constructor can only be automatically
// generated if no new properties are being declared
//
// class A extends B {
//   constructor(...args) = super(...args)
// }
void DesugarPass::inspect_leave(const ref<Class>& node) {
  if (node->constructor.get() == nullptr) {
    if (node->parent) {
      ref<CallOp> super_call = make<CallOp>(make<Super>(), make<Spread>(make<Id>("args")));
      ref<Function> constructor = make<Function>(false, make<Name>("constructor"), make<Block>(super_call));
      constructor->class_constructor = true;
      constructor->arguments.push_back(make<FunctionArgument>(false, true, make<Name>("args")));
      node->constructor = cast<Function>(apply(constructor));
    } else {
      ref<Function> constructor = make<Function>(false, make<Name>("constructor"), make<Block>());
      constructor->class_constructor = true;

      // initialize member variables
      for (const ref<ClassProperty>& prop : node->member_properties) {
        constructor->arguments.push_back(make<FunctionArgument>(true, false, prop->name, prop->value));
      }

      node->constructor = cast<Function>(apply(constructor));
    }
  }
}

// transform for-statements into their desugared form of using
// the builtin iterator methods and a while loop
//
// for const (a, b, c) in foo.bar() {
//   print(a, b, c)
// }
//
// into the desugared version:
//
// {
//   const __iterator = castiterator(foo.bar())
//   loop {
//     const (__value, __done) = iteratornext(__iterator)
//     if __done break
//     {
//       const (a, b, c) = __value
//       {
//         print(a, b, c)
//       }
//     }
//   }
// }
ref<Statement> DesugarPass::transform(const ref<For>& node) {
  ref<Block> block = make<Block>();

  // extract the source value from the for declaration
  ref<Expression> source;
  if (auto declaration = cast<Declaration>(node->declaration)) {
    source = declaration->expression;
  } else if (auto unpack_declaration = cast<UnpackDeclaration>(node->declaration)) {
    source = unpack_declaration->expression;
  } else {
    FAIL("unexpected node type");
  }

  // instantiate __iterator
  ref<BuiltinOperation> iterator_source = make<BuiltinOperation>(ir::BuiltinId::castiterator, source);
  ref<Declaration> iterator = make<Declaration>("__iterator", iterator_source, true);

  // loop block
  ref<Block> loop_block = make<Block>();
  ref<While> loop = make<While>(make<Bool>(true), loop_block);

  // build iterator result unpack declaration
  ref<UnpackTarget> unpack_target = make<UnpackTarget>(false, make<UnpackTargetElement>(make<Name>("__value")),
                                                       make<UnpackTargetElement>(make<Name>("__done")));
  ref<BuiltinOperation> unpack_source = make<BuiltinOperation>(ir::BuiltinId::iteratornext, make<Id>("__iterator"));
  ref<UnpackDeclaration> unpack_declaration = make<UnpackDeclaration>(unpack_target, unpack_source, true);
  loop_block->statements.push_back(unpack_declaration);

  // break if __done
  ref<If> break_if = make<If>(make<Id>("__done"), make<Block>(make<Break>()));
  loop_block->statements.push_back(break_if);

  // original body
  ref<Block> body_block = make<Block>();
  loop_block->statements.push_back(body_block);

  // re-use the original declaration node but replace the source
  if (auto dec = cast<Declaration>(node->declaration)) {
    dec->expression = make<Id>("__value");
  } else if (auto unpack_dec = cast<UnpackDeclaration>(node->declaration)) {
    unpack_dec->expression = make<Id>("__value");
  } else {
    FAIL("unexpected node type");
  }

  body_block->statements.push_back(node->declaration);
  body_block->statements.push_back(node->stmt);

  block->statements.push_back(iterator);
  block->statements.push_back(loop);

  return block;
}

ref<Statement> DesugarPass::transform(const ref<Switch>& node) {
  auto block = make<Block>();
  block->allows_break = true;

  auto decl = make<Declaration>("__charly_compiler_switch_test", node->test, true);
  block->push_back(decl);

  for (auto& case_node : node->cases) {
    auto test_var = make<Id>("__charly_compiler_switch_test");
    auto comparison = make<BinaryOp>(TokenType::Equal, test_var, case_node->test);
    comparison->set_location(case_node->test);
    auto if_stmt = make<If>(comparison, make<Block>(case_node->block, make<Break>()));
    block->push_back(if_stmt);
  }

  if (auto default_block = cast<Block>(node->default_block)) {
    block->push_back(default_block);
    block->push_back(make<Break>());
  }

  return block;
}

}  // namespace charly::core::compiler::ast
