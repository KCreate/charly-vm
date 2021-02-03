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

#include <queue>
#include <sstream>
#include <vector>

#include <catch2/catch_all.hpp>

#include "charly/core/compiler/pass.h"
#include "charly/core/compiler/parser.h"

using namespace charly;

#include "astmacros.h"

struct VisitedNodesStatisticsPass : public Pass {
  int types[256] = { 0 };

  virtual void enter(const ref<Node>& node) override {
    types[static_cast<int>(node->type())] += 1;
  }
};

struct NumberSummerPass : public Pass {
  int64_t intsum = 0;
  double floatsum = 0.0;

  virtual void inspect_leave(const ref<Int>& node) override {
    intsum += node->value;
  }

  virtual void inspect_leave(const ref<Float>& node) override {
    floatsum += node->value;
  }
};

TEST_CASE("visits each node") {
  ref<Expression> node1 = EXP("(1, (2.5, 3), 4.25, (5, (((6.75, 7))), 8.1555), 9)", Expression);

  VisitedNodesStatisticsPass visited_stat_pass;
  visited_stat_pass.apply(node1);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Int)] == 5);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Float)] == 4);
  CHECK(visited_stat_pass.types[static_cast<int>(Node::Type::Tuple)] == 4);

  NumberSummerPass summer_pass;
  summer_pass.apply(node1);
  CHECK(summer_pass.intsum == 25);
  CHECK(summer_pass.floatsum == 21.6555);
}

TEST_CASE("can modify ast nodes") {
  ref<Expression> node1 = EXP("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)", Expression);

  struct IntsAboveFiveWithZeroReplacerPass : public Pass {
    virtual ref<Expression> transform(const ref<Int>& node) override {
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

  CHECK(summer.intsum == 15);
  CHECK(summer.floatsum == 0);
}

TEST_CASE("can replace nodes") {
  ref<Expression> node1 = EXP("(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)", Expression);

  struct IntsThatAreFiveOrAboveWithPiReplacerPass : public Pass {
    virtual ref<Expression> transform(const ref<Int>& node) override {
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

  CHECK(summer.intsum == 10);
  CHECK(summer.floatsum == 3.1415 * 6);
}

TEST_CASE("can remove statements from blocks and tuples") {
  ref<Block> block = cast<Block>(Parser::parse_program("1 2 3 4")->body);

  struct IntsAbove2RemoverPass : public Pass {
    virtual ref<Expression> transform(const ref<Int>& node) override {
      if (node->value > 2) {
        return nullptr;
      }

      return node;
    }
  };

  CHECK(block->statements.size() == 4);
  IntsAbove2RemoverPass().apply(block);
  CHECK(block->statements.size() == 2);

  ref<Tuple> tuple = cast<Tuple>(EXP("(1, 2, 3, 4)", Tuple));
  CHECK(tuple->elements.size() == 4);
  IntsAbove2RemoverPass().apply(tuple);
  CHECK(tuple->elements.size() == 2);
}

TEST_CASE("calls enter and leave callbacks") {
  ref<Expression> exp = EXP("((1, 2), (3, 4))", Expression);

  struct OrderVerifyPass : public Pass {
    std::vector<Node::Type> typestack;

    virtual void enter(const ref<Node>& node) override {
      typestack.push_back(node->type());
    }

    virtual void leave(const ref<Node>& node) override {
      CHECK(typestack.back() == node->type());
      typestack.pop_back();
    }
  };

  OrderVerifyPass verify_pass;
  verify_pass.apply(exp);
}

TEST_CASE("enter method can prevent children from being visited") {
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

    bool finished() {
      return queued_tuples.size() == 0;
    }

    virtual bool inspect_enter(const ref<Tuple>& block) override {
      queued_tuples.push(block);
      return false;
    }

    virtual void inspect_leave(const ref<Int>& node) override {
      visited_ints.push_back(node);
    }
  };

  TupleSequencerPass sequencer;

  sequencer.apply(EXP("((((((0,), 1), 2), 3), 4), 5)", Expression));

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
