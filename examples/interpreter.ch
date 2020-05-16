/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

class Lexer {
  property tokens
  property source
  property pos
  property buffer

  func constructor() {
    @tokens = []
    @token = null
    @source = ""
    @pos = 0
    @buffer = ""
  }

  func setup(source) {
    @tokens = []
    @source = source
    @pos = 0
  }

  func tokenize(source) {
    @setup(source)

    while @pos < @source.length {
      const token = @read_token()
      if token @tokens.push(token)
    }

    @tokens
  }

  func read_token() {
    let char = @current_char()

    guard typeof char == "string" {
      return false
    }

    if char == " " {
      @read_char()
      return false
    }

    if char == "(" {
      @read_char()
      return {
        type: "LeftParen"
      }
    }

    if char == ")" {
      @read_char()
      return {
        type: "RightParen"
      }
    }

    if char == "+" || char == "-" || char == "*" || char == "/" {
      @read_char()
      return {
        type: "Operator",
        value: char
      }
    }

    if char.is_digit() {
      return @read_numeric()
    }

    throw "Unrecognized char: " + char
  }

  func read_char() {
    @source[@pos + 1].tap(->{
      @pos += 1
    })
  }

  func peek_char() {
    @source[@pos + 1]
  }

  func current_char() {
    @source[@pos]
  }

  func read_numeric() {
    @buffer = ""

    loop {
      const char = @current_char()

      if typeof char ! "string" {
        @read_char()
        break
      }

      if char.is_digit() {
        @buffer += char
        @read_char()
      } else {
        break
      }
    }

    const numeric_value = @buffer.to_n().tap(->{
      @buffer = ""
    })

    return {
      type: "Numeric",
      value: numeric_value
    }
  }
}

class Parser {
  property lexer
  property tokens
  property pos

  func constructor() {
    @lexer = new Lexer()
    @tokens = []
    @pos = -1
  }

  func parse_error(expected, real) {
    if typeof real == "null" {
      throw "Expected " + expected + " but reached end of input"
    }

    throw "Expected " + expected + " but got " + real.type
  }

  func setup(source) {
    @pos = -1
    @tokens = @lexer.tokenize(source)
    @advance()
  }

  func parse(source) {
    @setup(source)
    @parse_expression()
  }

  func advance() {
    @token = @tokens[@pos + 1].tap(->{
      @pos += 1
    })
  }

  func parse_expression() {
    @parse_addition()
  }

  func parse_addition() {
    let left = @parse_multiplication()

    while @token.type == "Operator" {
      if @token.value == "+" || @token.value == "-" {
        let operator = @token.value
        @advance()

        let node = {}
        node.left = left
        node.right = @parse_multiplication()
        node.operator = operator
        node.type = "Operation"
        left = node
      } else {
        break
      }
    }

    return left
  }

  func parse_multiplication() {
    let left = @parse_literal()

    while @token.type == "Operator" {
      if @token.value == "*" || @token.value == "/" {
        let operator = @token.value
        @advance()

        let node = {}
        node.left = left
        node.right = @parse_literal()
        node.operator = operator
        node.type = "Operation"
        left = node
      } else {
        break
      }
    }

    return left
  }

  func parse_literal() {
    if @token.type == "Numeric" {
      let node = {}
      node.type = "NumericLiteral"
      node.value = @token.value
      @advance()
      return node
    }

    if @token.type == "LeftParen" {
      @advance()
      let node = @parse_expression()

      if @token.type == "RightParen" {
        @advance()
        return node
      } else {
        @parse_error(")", @token)
      }
    }

    @parse_error("literal", @token)
  }
}

class Visitor {
  property tree

  func constructor() {
    @tree = {}
  }

  func execute(tree) {
    @tree = tree

    @visit(tree)
  }

  func visit(node) {
    if node.type == "NumericLiteral" {
      return node.value
    }

    if node.type == "Operation" {
      let left = @visit(node.left)
      let right = @visit(node.right)

      if node.operator == "+" {
        return left + right
      } else if node.operator == "-" {
        return left - right
      } else if node.operator == "*" {
        return left * right
      } else if node.operator == "/" {
        return left / right
      }
    }
  }
}

const parser = new Parser()
const visitor = new Visitor()

try {
  const input = "(250 * 4) * (4 / 32) / 25 + 415"
  print(input)
  const tree = parser.parse(input)
  print(tree)
  const result = visitor.execute(tree)
  print(result)
} catch(e) {
  print("Error:")
  print(e)
}
