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

#include <sstream>
#include <vector>
#include <queue>

#include <catch2/catch_all.hpp>

#include "charly/core/compiler/parser.h"
#include "charly/core/compiler/astpass.h"

using Catch::Matchers::Contains;

using namespace charly;
using namespace charly::core::compiler;

#define ASSERT_AST_TYPE(S, T)                          \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
  }

#define ASSERT_AST_VALUE(S, T, V)                      \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
    if (isa<T>(exp)) {                                 \
      CHECK(cast<T>(exp)->value == V);                 \
    }                                                  \
  }

#define ASSERT_AST_STRING(S, T, V)                     \
  {                                                    \
    ref<Expression> exp = Parser::parse_expression(S); \
    CHECK(isa<T>(exp));                                \
    if (isa<T>(exp)) {                                 \
      CHECK(cast<T>(exp)->value.compare(V) == 0);      \
    }                                                  \
  }

#define EXP(S, T) cast<T>(Parser::parse_expression(S))

struct VisitedNodesStatisticsPass : public ASTPass {
  int types[256] = {0};

  virtual bool enter_any(const ref<Node>& node) override {
    types[static_cast<int>(node->type())] += 1;
    return true;
  }
};

struct NumberSummerPass : public ASTPass {
    int64_t intsum = 0;
    double floatsum = 0.0;

    virtual ref<Expression> leave(const ref<Int>& node) override {
      intsum += node->value;
      return node;
    }

    virtual ref<Expression> leave(const ref<Float>& node) override {
      floatsum += node->value;
      return node;
    }
};

TEST_CASE("visits each node") {
  ref<Expression> node1 = Parser::parse_expression("(1, (2.5, 3), 4.25, (5, (((6.75, 7))), 8.1555), 9)");


  VisitedNodesStatisticsPass visited_stat_pass;
  visited_stat_pass.visit(node1);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Int)] == 5);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Float)] == 4);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Tuple)] == 4);

  NumberSummerPass summer_pass;
  summer_pass.visit(node1);
  CHECK(summer_pass.intsum == 25);
  CHECK(summer_pass.floatsum == 21.6555);
}

TEST_CASE("can modify ast nodes") {
  ref<Expression> node1 = Parser::parse_expression("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");

  struct IntsAboveFiveWithZeroReplacerPass : public ASTPass {
    virtual ref<Expression> leave(const ref<Int>& node) override {
      if (node->value > 5) {
        node->value = 0;
      }

      return node;
    }
  };

  IntsAboveFiveWithZeroReplacerPass replacer;
  replacer.visit(node1);

  NumberSummerPass summer;
  summer.visit(node1);

  CHECK(summer.intsum == 15);
  CHECK(summer.floatsum == 0);
}

TEST_CASE("can replace nodes") {
  ref<Expression> node1 = Parser::parse_expression("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");

  struct IntsThatAreFiveOrAboveWithPiReplacerPass : public ASTPass {
    virtual ref<Expression> leave(const ref<Int>& node) override {
      if (node->value >= 5) {
        return make<Float>(3.1415);
      }

      return node;
    }
  };

  IntsThatAreFiveOrAboveWithPiReplacerPass replacer;
  replacer.visit(node1);

  NumberSummerPass summer;
  summer.visit(node1);

  CHECK(summer.intsum == 10);
  CHECK(summer.floatsum == 3.1415 * 6);
}

TEST_CASE("can remove statements from blocks and tuples") {
  ref<Block> block = cast<Block>(Parser::parse_program("1 2 3 4")->body);

  struct IntsAbove2RemoverPass : public ASTPass {
    virtual ref<Expression> leave(const ref<Int>& node) override {
      if (node->value > 2)
        return nullptr;

      return node;
    }
  };

  CHECK(block->statements.size() == 4);
  IntsAbove2RemoverPass().visit(block);
  CHECK(block->statements.size() == 2);

  ref<Tuple> tuple = cast<Tuple>(Parser::parse_expression("(1, 2, 3, 4)"));
  CHECK(tuple->elements.size() == 4);
  IntsAbove2RemoverPass().visit(tuple);
  CHECK(tuple->elements.size() == 2);
}

TEST_CASE("calls enter and leave callbacks") {
  ref<Expression> exp = Parser::parse_expression("((1, 2), (3, 4))");

  struct OrderVerifyPass : public ASTPass {
    std::vector<Node::Type> typestack;

    virtual bool enter_any(const ref<Node>& node) override {
      typestack.push_back(node->type());
      return true;
    }

    virtual ref<Node> leave_any(const ref<Node>& node) override {
      CHECK(typestack.back() == node->type());
      typestack.pop_back();
      return node;
    }
  };

  OrderVerifyPass verify_pass;
  verify_pass.visit(exp);
}

TEST_CASE("enter method can prevent children from being visited") {
  struct TupleSequencerPass : public ASTPass {
    std::vector<ref<Int>> visited_ints;
    std::queue<ref<Tuple>> queued_tuples;

    void keep_processing() {
      ref<Tuple> tup = queued_tuples.front();
      queued_tuples.pop();

      for (ref<Expression>& exp : tup->elements) {
        visit(exp);
      }
    }

    bool finished() {
      return queued_tuples.size() == 0;
    }

    virtual ref<Expression> leave(const ref<Int>& node) override {
      visited_ints.push_back(node);
      return node;
    }

    virtual bool enter(const ref<Tuple>& block) override {
      queued_tuples.push(block);
      return false;
    }
  };

  TupleSequencerPass sequencer;

  sequencer.visit(Parser::parse_expression("((((((0,), 1), 2), 3), 4), 5)"));

  do {
    sequencer.keep_processing();
  } while (!sequencer.finished());

  CHECK(sequencer.visited_ints.size() == 6);
  CHECK(sequencer.visited_ints[0]->value == 5);
  CHECK(sequencer.visited_ints[1]->value == 4);
  CHECK(sequencer.visited_ints[2]->value == 3);
  CHECK(sequencer.visited_ints[3]->value == 2);
  CHECK(sequencer.visited_ints[4]->value == 1);
  CHECK(sequencer.visited_ints[5]->value == 0);
}

TEST_CASE("counts how many nodes have been modified") {
  ref<Tuple> tup = EXP("((((5,),),),)", Tuple);

  struct RemoveTopTuplePass : public ASTPass {
    virtual bool enter(const ref<Tuple>&) override {
      return false;
    }

    virtual ref<Expression> leave(const ref<Tuple>& node) override {
      REQUIRE(node->elements.size() == 1);
      return node->elements[0];
    }
  };

  RemoveTopTuplePass pass;

  CHECK(pass.modified_count() == 0);

  ref<Expression> result;
  result = pass.visit(tup);
  result = pass.visit(result);
  result = pass.visit(result);
  result = pass.visit(result);

  CHECK(pass.modified_count() == 4);

  REQUIRE(isa<Int>(result));
  CHECK(cast<Int>(result)->value == 5);
}
