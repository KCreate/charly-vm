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

// the following code implements a crude and shitty x86 compiler.
// you feed it an AST (see examples below) and it outputs 32-bit x86 assembly directly
// to the console.
//
// it supports function arguments, local variables, assignments, arithmetic (add, sub,
// mul, div) and a return statement.
//
// code can be executed by pasting the output of this code into the following website:
// https://carlosrafaelgn.com.br/asm86/
//
// insert the following assembly at the beginning to call individual functions
// else it won't work. you can call methods with up to 5 arguments by storing the arguments
// in eax, ebx, ecx, edx and edi before calling the function
//
//  main:
//    mov eax, 0x01   ; first argument
//    mov ebx, 0x02   ; second argument
//    call foo        ; the function to call
//    ret             ; will cause an invalid memory load bc no previous frame

class Context {
  property current_function
}

class ASTNode {
  compile {
    throw Error("compile method not implemented for " + self.klass)
  }
}

class Program extends ASTNode {
  property functions
  constructor(@functions) = super()

  compile {
    @functions.each(->(f) {
      const ctx = Context(f)
      f.compile(ctx)
    })
  }
}

class FunctionDef extends ASTNode {
  property name
  property locals
  property parameters
  property body
  constructor(@name, @locals, parameters, @body) {
    super()

    if parameters.length > 5 throw Error("too many parameters for function " + name)
    @parameters = parameters
  }

  calculate_framesize = @locals.length * 8 + @parameters.length * 8

  local_stackoffset(name) {
    const parameterindex = @parameters.index(name)
    if parameterindex >= 0 {
      return parameterindex * 8
    }

    const localoffset = @parameters.length * 8
    const localindex = @locals.index(name)
    if localindex >= 0 {
      return localoffset + localindex * 8
    }

    throw Error("unknown identifier " + name)
  }

  compile(ctx) {

    // function label
    print(@name, ":")

    // function prologue
    print("  push ebp")
    print("  sub esp, ", @calculate_framesize())
    print("  mov ebp, esp")
    print("")

    // copy function arguments into stack
    const registers = [
      "eax",
      "ebx",
      "ecx",
      "edx",
      "edi"
    ]
    @parameters.each(->(p, i) {
      const offset = i * 8
      print("  mov [ebp + " + offset + "], " + registers[i])
      print("; " + offset + " = " + p)
    })
    print("")

    // compile body
    @body.compile(ctx)

    // body final return statement generates the function epilogue
    print("")
  }
}

class Block extends ASTNode {
  property statements
  constructor(@statements) = super()

  compile(ctx) {
    @statements.each(->(s) {
      s.compile(ctx)
      print("")
    })
  }
}

class Identifier extends ASTNode {
  property name
  constructor(@name) = super()

  compile(ctx) {
    const offset = ctx.current_function.local_stackoffset(@name)
    print("  mov eax, [ebp + " + offset + "]")
  }
}

class IntNode extends ASTNode {
  property value
  constructor(@value) = super()

  compile {
    print("mov eax, " + value)
  }
}

class BinaryOp extends ASTNode {
  property operation
  property left
  property right
  constructor(@operation, @left, @right) = super()

  compile(ctx) {
    @left.compile(ctx)
    print("  push eax")
    @right.compile(ctx)
    print("  pop ebx")

    switch @operation {
      case "add" {
        print("  add eax, ebx")
      }

      case "sub" {
        print("  sub eax, ebx")
      }

      case "mul" {
        print("  mul ebx")
      }

      case "div" {
        print("  div eax, ebx")
      }
    }
  }
}

class Assignment extends ASTNode {
  property target
  property value
  constructor(@target, @value) = super()

  compile(ctx) {
    const target_stack_offset = ctx.current_function.local_stackoffset(@target)
    @value.compile(ctx)

    print("  mov [ebp + " + target_stack_offset + "], eax")
  }
}

class ReturnStmt extends ASTNode {
  property value
  constructor(@value) = super()

  compile(ctx) {
    @value.compile(ctx)

    // function epilogue
    print("")
    print("  add esp, ", ctx.current_function.calculate_framesize())
    print("  pop ebp")
    print("  ret")
  }
}

const tree = Program([
  /*
  int foo(int a, int b) {
    int tmp = a + b;
    tmp = tmp * a;
    return tmp * b;
  }
  */
  FunctionDef(
    "foo",
    ["tmp"],
    ["a", "b"],
    Block([
      Assignment(
        "tmp",
        BinaryOp(
          "add",
          Identifier("a"),
          Identifier("b")
        )
      ),
      Assignment(
        "tmp",
        BinaryOp(
          "mul",
          Identifier("tmp"),
          Identifier("a")
        )
      ),
      ReturnStmt(
        BinaryOp(
          "mul",
          Identifier("tmp"),
          Identifier("b")
        )
      )
    ])
  ),

  /*
  int square(int a) {
    return a * a;
  }
  */
  FunctionDef(
    "square",
    [],
    ["a"],
    Block([
      ReturnStmt(
        BinaryOp(
          "mul",
          Identifier("a"),
          Identifier("a")
        )
      )
    ])
  ),

  /*
  int longassfunc(int a, int b, int c) {
    int tmp = a * b;
    int tmp2 = b * c;
    tmp = tmp / tmp2
    tmp = tmp + a * b;
    return tmp + (a + (b + c));
  }
  */
  FunctionDef(
    "longassfunc",
    ["tmp", "tmp2"],
    ["a", "b", "c"],
    Block([
      Assignment(
        "tmp",
        BinaryOp(
          "mul",
          Identifier("a"),
          Identifier("b")
        )
      ),
      Assignment(
        "tmp2",
        BinaryOp(
          "mul",
          Identifier("b"),
          Identifier("c")
        )
      ),
      Assignment(
        "tmp",
        BinaryOp(
          "div",
          Identifier("tmp"),
          Identifier("tmp2")
        )
      ),
      Assignment(
        "tmp",
        BinaryOp(
          "add",
          Identifier("tmp"),
          BinaryOp(
            "mul",
            Identifier("a"),
            Identifier("b")
          )
        )
      ),
      ReturnStmt(
        BinaryOp(
          "add",
          Identifier("tmp"),
          BinaryOp(
            "add",
            Identifier("a"),
            BinaryOp(
              "add",
              Identifier("b"),
              Identifier("c")
            )
          )
        )
      )
    ])
  )
])

tree.compile()
