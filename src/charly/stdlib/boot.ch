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

func determine_max_stack_size {
    let counter = 0

    try {
        func recurse {
            counter += 1
            recurse()
        }
        recurse()
    } catch(e) {
        return counter
    }

    return 100
}

const builtin_setbuiltinclass = @"charly.builtin.core.setbuiltinclass"
const builtin_writevalue = @"charly.builtin.core.writevalue"
const builtin_writeline = @"charly.builtin.core.writeline"
const builtin_writevaluesync = @"charly.builtin.core.writevaluesync"
const builtin_readfile = @"charly.builtin.core.readfile"
const builtin_compile = @"charly.builtin.core.compile"
const builtin_disassemble = @"charly.builtin.core.disassemble"
const builtin_makelist = @"charly.builtin.core.makelist"
const builtin_exit = @"charly.builtin.core.exit"

const builtin_readline_prompt = @"charly.builtin.readline.prompt"
const builtin_readline_add_history = @"charly.builtin.readline.add_history"
const builtin_readline_clear_history = @"charly.builtin.readline.clear_history"

func write(string) = builtin_writevalue("{string}\n")
func writeline(string) = builtin_writeline(string)
func writesync(string) = builtin_writevaluesync(string)

func echo(string) {
    writesync(string);
}

func prompt(message = "> ", append_to_history = true) {
    const result = builtin_readline_prompt("{message}")

    if append_to_history {
        builtin_readline_add_history(result)
    }

    return result
}

func add_history(message) = builtin_readline_add_history(message)

func clear_history = builtin_readline_clear_history()

func exit(status = 0) = builtin_exit(status)

func compile(source, name = "repl") = builtin_compile(source, name)

func readfile(name) = builtin_readfile(name)

func dis(function) {
    echo("disassembly of {function}")
    builtin_disassemble(function)
}

func list(size = 0, initial = null) {
    return builtin_makelist(size, initial)
}

class Value extends @"charly.baseclass" {}
class Number extends Value {}
class Int extends Number {}
class Float extends Number {}
class Bool extends Value {}
class Symbol extends Value {}
class Null extends Value {}
class String extends Value {}
class Bytes extends Value {}

class Instance extends Value {
    property __klass

    func constructor = super()
}

class Tuple extends Value {}

class Class extends Value {
    property name
    property parent
    property __shape
    property __function_table
    property __constructor

    func constructor = super()
}

class Shape extends Value {
    property __own_shape_id
    property parent
    property __key_table

    func constructor = super()
}

class Function extends Value {
    property __context
    property __shared_info

    func constructor = super()
}

class BuiltinFunction extends Value {
    property __function_ptr
    property name
    property __argc

    func constructor = super()
}

class Fiber extends Value {
    property __thread
    property function
    property context
    property arguments
    property result

    func constructor = super()
}

class Exception extends Value {
    property message
    property stack_trace

    func constructor = super()
}

builtin_setbuiltinclass(@"charly.shapeid.int", Int)
builtin_setbuiltinclass(@"charly.shapeid.float", Float)
builtin_setbuiltinclass(@"charly.shapeid.bool", Bool)
builtin_setbuiltinclass(@"charly.shapeid.symbol", Symbol)
builtin_setbuiltinclass(@"charly.shapeid.null", Null)
builtin_setbuiltinclass(@"charly.shapeid.small_string", String)
builtin_setbuiltinclass(@"charly.shapeid.small_bytes", Bytes)
builtin_setbuiltinclass(@"charly.shapeid.large_string", String)
builtin_setbuiltinclass(@"charly.shapeid.large_bytes", Bytes)
builtin_setbuiltinclass(@"charly.shapeid.user_instance", Instance)
builtin_setbuiltinclass(@"charly.shapeid.huge_bytes", Bytes)
builtin_setbuiltinclass(@"charly.shapeid.huge_string", String)
builtin_setbuiltinclass(@"charly.shapeid.tuple", Tuple)
builtin_setbuiltinclass(@"charly.shapeid.class", Class)
builtin_setbuiltinclass(@"charly.shapeid.shape", Shape)
builtin_setbuiltinclass(@"charly.shapeid.function", Function)
builtin_setbuiltinclass(@"charly.shapeid.builtin_function", BuiltinFunction)
builtin_setbuiltinclass(@"charly.shapeid.fiber", Fiber)
builtin_setbuiltinclass(@"charly.shapeid.exception", Exception)

let $$ = null
let @"charly.boot" = func boot {
    @"charly.boot" = null

    func execute_program(filename) {
        const file = readfile(filename)
        if file == null {
            throw "could not open file {filename}"
        }

        const module = compile(file, filename)
        if module == null {
            throw "could not compile {filename}"
        }

        return module()
    }

    const filename = ARGV[0]

    if filename != null {
        return execute_program(filename)
    }

    let repl_source_counter = 0
    func compile(source, name) {
        repl_source_counter += 1
        return builtin_compile(source, name)
    }

    let input = ARGV[0]

    if input == null {
        input = prompt()
    }

    loop {
        switch input {
            case "" {
                break
            }

            case ".exit" {
                return
            }

            case ".clear" {
                clear_history()
                break
            }

            case ".help" {
                echo(".exit        Exit from the REPL")
                echo(".clear       Clear REPL history")
                echo(".help        Show this help message")
                break
            }

            default {
                try {
                    const module = compile(input, "repl")
                    const fiber = spawn module()
                    $$ = await fiber
                } catch(e) {
                    $$ = e
                    echo("Caught exception:")
                }

                echo($$)
            }
        }

        input = prompt()
    }
}

return @"charly.boot"()