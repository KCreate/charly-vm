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

#include "charly/core/compiler/passes/desugar_pass.h"
#include "charly/utils/cast.h"

namespace charly::core::compiler::ast {

ref<Statement> DesugarPass::transform(const ref<Block>& node) {

  // unwrap block(block(...))
  if (node->statements.size() == 1) {
    if (ref<Block> block = cast<Block>(node->statements.front())) {
      return block;
    }
  }

  return node;
}

void DesugarPass::inspect_leave(const ref<Import>& node) {
  ref<Expression> source = node->source;

  if (ref<Name> name = cast<Name>(source)) {
    source = make<String>(name->value);
  }

  if (!isa<String>(source)) {
    source = make<BuiltinOperation>(ir::BuiltinId::caststring, source);
  }

  node->source = source;
}

ref<Expression> DesugarPass::transform(const ref<Spawn>& node) {
  ir::BuiltinId builtin_id = node->execute_immediately ? ir::BuiltinId::fiberspawn : ir::BuiltinId::fibercreate;
  ref<BuiltinOperation> builtin = make<BuiltinOperation>(builtin_id);

  ref<Expression> function;
  ref<Tuple> arguments = make<Tuple>();

  switch (node->statement->type()) {

    // spawn { <body> } -> spawn(->{ <body> }, ())
    case Node::Type::Block: {
      function = make<Function>(true, make<Name>(""), cast<Block>(node->statement));
      function->set_location(node);
      break;
    }

    // spawn foo.bar(1, 2)    -> spawn(foo.bar, (1, 2))
    case Node::Type::CallOp: {
      ref<CallOp> call = cast<CallOp>(node->statement);

      function = call->target;

      for (const ref<Expression>& arg : call->arguments) {
        arguments->elements.push_back(arg);
      }

      break;
    }

    default: {
      assert(false && "unexpected node type");
      break;
    }
  }

  builtin->arguments.push_back(function);
  builtin->arguments.push_back(arguments);

  return builtin;
}

ref<Expression> DesugarPass::transform(const ref<FormatString>& node) {

  // wrap all non-string elements into a caststring node
  for (ref<Expression>& element : node->elements) {
    if (!(isa<String>(element) || isa<BuiltinOperation>(element))) {
      element = make<BuiltinOperation>(ir::BuiltinId::caststring, element);
    }
  }

  // if only a single value remains in the formatstring,
  // remove the formatstring entirely
  if (node->elements.size() == 1) {
    return node->elements.back();
  }

  return node;
}

void DesugarPass::inspect_leave(const ref<Function>& node) {

  // wrap regular functions with yield expressions inside a generator wrapper function
  if (!node->arrow_function) {
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
      ref<Function> func = make<Function>(true, make<Name>(""), node->body);

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
      ref<Spawn> spawn = make<Spawn>(func_call);
      spawn->execute_immediately = false;
      ref<BuiltinOperation> castiterator = make<BuiltinOperation>(ir::BuiltinId::castiterator, spawn);
      ref<Return> return_node = make<Return>(castiterator);
      ref<Block> new_body = make<Block>(return_node);

      node->body = cast<Block>(apply(new_body));
    }
  }
}

// generate default constructor for base classes
//
// class A {
//   property a = 1
//   property b = 2
//   property c = 3
//
//   constructor(@a, @b, @c)
// }
void DesugarPass::inspect_leave(const ref<Class>& node) {
  if (!node->parent) {
    if (node->constructor.get() == nullptr) {
      ref<Function> constructor = make<Function>(false, make<Name>("constructor"), make<Block>());
      constructor->class_constructor = true;

      // initialize member variables
      for (const ref<ClassProperty>& prop : node->member_properties) {
        constructor->arguments.push_back(make<FunctionArgument>(true, false, prop->name, prop->value));
      }

      node->constructor = constructor;
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
  } else if (auto declaration = cast<UnpackDeclaration>(node->declaration)) {
    source = declaration->expression;
  } else {
    assert(false && "unexpected node type");
  }

  // instantiate __iterator
  ref<BuiltinOperation> __iterator_source = make<BuiltinOperation>(ir::BuiltinId::castiterator, source);
  ref<Declaration> __iterator = make<Declaration>("__iterator", __iterator_source, true);

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
  if (auto declaration = cast<Declaration>(node->declaration)) {
    declaration->expression = make<Id>("__value");
  } else if (auto declaration = cast<UnpackDeclaration>(node->declaration)) {
    declaration->expression = make<Id>("__value");
  } else {
    assert(false && "unexpected node type");
  }

  body_block->statements.push_back(node->declaration);
  body_block->statements.push_back(node->stmt);

  block->statements.push_back(__iterator);
  block->statements.push_back(loop);

  return block;
}

}  // namespace charly::core::compiler::ast
