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

#include <queue>
#include <vector>

#include <catch2/catch_all.hpp>

#include "charly/core/compiler/parser.h"
#include "charly/core/compiler/pass.h"

using namespace charly;

#include "astmacros.h"

struct VisitedNodesStatisticsPass : public Pass {
  int types[256] = { 0 };

  void enter(const ref<Node>& node) override {
    types[static_cast<int>(node->type())] += 1;
  }
};

struct NumberSummerPass : public Pass {
  int64_t intsum = 0;
  double floatsum = 0.0;

  void inspect_leave(const ref<Int>& node) override {
    intsum += node->value;
  }

  void inspect_leave(const ref<Float>& node) override {
    floatsum += node->value;
  }
};

CATCH_TEST_CASE("Pass") {
  CATCH_SECTION("visits each node") {
    ref<Expression> node1 = EXP<Expression>("(1, (2.5, 3), 4.25, (5, (((6.75, 7))), 8.1555), 9)");

    VisitedNodesStatisticsPass visited_stat_pass;
    visited_stat_pass.apply(node1);
    CATCH_CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Int)] == 5);
    CATCH_CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Float)] == 4);
    CATCH_CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Tuple)] == 4);

    NumberSummerPass summer_pass;
    summer_pass.apply(node1);
    CATCH_CHECK(summer_pass.intsum == 25);
    CATCH_CHECK(summer_pass.floatsum == 21.6555);
  }

  CATCH_SECTION("can modify ast nodes") {
    ref<Expression> node1 = EXP<Expression>("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");

    struct IntsAboveFiveWithZeroReplacerPass : public Pass {
      ref<Expression> transform(const ref<Int>& node) override {
        if (node->value > 5) {
          node->value = 0;
        }

        return node;
      }
    };

    IntsAboveFiveWithZeroReplacerPass replacer;
    replacer.apply(node1);

    NumberSummerPass summer;
    summer.apply(node1);

    CATCH_CHECK(summer.intsum == 15);
    CATCH_CHECK(summer.floatsum == 0);
  }

  CATCH_SECTION("can replace nodes") {
    ref<Expression> node1 = EXP<Expression>("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)");

    struct IntsThatAreFiveOrAboveWithPiReplacerPass : public Pass {
      ref<Expression> transform(const ref<Int>& node) override {
        if (node->value >= 5) {
          return make<Float>(3.1415);
        }

        return node;
      }
    };

    IntsThatAreFiveOrAboveWithPiReplacerPass replacer;
    replacer.apply(node1);

    NumberSummerPass summer;
    summer.apply(node1);

    CATCH_CHECK(summer.intsum == 10);
    CATCH_CHECK(summer.floatsum == 3.1415 * 6);
  }

  CATCH_SECTION("can remove statements from blocks and tuples") {
    ref<Block> block = cast<Block>(Parser::parse_program("1 2 3 4"));

    struct IntsAbove2RemoverPass : public Pass {
      ref<Expression> transform(const ref<Int>& node) override {
        if (node->value > 2) {
          return nullptr;
        }

        return node;
      }
    };

    CATCH_CHECK(block->statements.size() == 4);
    IntsAbove2RemoverPass().apply(block);
    CATCH_CHECK(block->statements.size() == 2);

    ref<Tuple> tuple = cast<Tuple>(EXP<Tuple>("(1, 2, 3, 4)"));
    CATCH_CHECK(tuple->elements.size() == 4);
    IntsAbove2RemoverPass().apply(tuple);
    CATCH_CHECK(tuple->elements.size() == 2);
  }

  CATCH_SECTION("calls enter and leave callbacks") {
    ref<Expression> exp = EXP<Expression>("((1, 2), (3, 4))");

    struct OrderVerifyPass : public Pass {
      std::vector<Node::Type> typestack;

      void enter(const ref<Node>& node) override {
        typestack.push_back(node->type());
      }

      void leave(const ref<Node>& node) override {
        CATCH_CHECK(typestack.back() == node->type());
        typestack.pop_back();
      }
    };

    OrderVerifyPass verify_pass;
    verify_pass.apply(exp);
  }

  CATCH_SECTION("enter method can prevent children from being visited") {
    struct TupleSequencerPass : public Pass {
      std::vector<ref<Int>> visited_ints;
      std::queue<ref<Tuple>> queued_tuples;

      void keep_processing() {
        ref<Tuple> tup = queued_tuples.front();
        queued_tuples.pop();

        for (ref<Expression>& exp : tup->elements) {
          apply(exp);
        }
      }

      bool finished() const {
        return queued_tuples.empty();
      }

      bool inspect_enter(const ref<Tuple>& block) override {
        queued_tuples.push(block);
        return false;
      }

      void inspect_leave(const ref<Int>& node) override {
        visited_ints.push_back(node);
      }
    };

    TupleSequencerPass sequencer;

    sequencer.apply(EXP<Expression>("((((((0,), 1), 2), 3), 4), 5)"));

    do {
      sequencer.keep_processing();
    } while (!sequencer.finished());

    CATCH_CHECK(sequencer.visited_ints.size() == 6);
    CATCH_CHECK(sequencer.visited_ints[0]->value == 5);
    CATCH_CHECK(sequencer.visited_ints[1]->value == 4);
    CATCH_CHECK(sequencer.visited_ints[2]->value == 3);
    CATCH_CHECK(sequencer.visited_ints[3]->value == 2);
    CATCH_CHECK(sequencer.visited_ints[4]->value == 1);
    CATCH_CHECK(sequencer.visited_ints[5]->value == 0);
  }
}
