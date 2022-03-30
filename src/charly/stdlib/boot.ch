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

const builtin_currentfiber = @"charly.builtin.core.currentfiber"
const builtin_transplant_builtin_class = @"charly.builtin.core.transplantbuiltinclass"
const builtin_writevalue = @"charly.builtin.core.writevalue"
const builtin_writeline = @"charly.builtin.core.writeline"
const builtin_writevaluesync = @"charly.builtin.core.writevaluesync"
const builtin_readfile = @"charly.builtin.core.readfile"
const builtin_getstacktrace = @"charly.builtin.core.getstacktrace"
const builtin_compile = @"charly.builtin.core.compile"
const builtin_disassemble = @"charly.builtin.core.disassemble"
const builtin_makelist = @"charly.builtin.core.makelist"
const builtin_exit = @"charly.builtin.core.exit"

const builtin_readline_prompt = @"charly.builtin.readline.prompt"
const builtin_readline_add_history = @"charly.builtin.readline.add_history"
const builtin_readline_clear_history = @"charly.builtin.readline.clear_history"

func write(string) = builtin_writevalue("{string}\n")
func print(string) = builtin_writeline(string)
func writesync(string) = builtin_writevaluesync(string)

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

class builtin_Instance {
    func constructor = self
}
builtin_transplant_builtin_class(Instance, builtin_Instance)

class builtin_Tuple {
    static func make(length = 0, initial = null) = builtin_makelist(length, initial)
}
builtin_transplant_builtin_class(Tuple, builtin_Tuple)

class builtin_Function {
    func disassemble {
        print("disassembly of {self}")
        builtin_disassemble(self)
    }
}
builtin_transplant_builtin_class(Function, builtin_Function)

class builtin_Exception {
    func constructor(@message) {
        // TODO: trim correct numer of stack frames when called from subclass constructors
        @stack_trace = Exception.getstacktrace(2)
    }

    static func getstacktrace(trim = 0) = builtin_getstacktrace(trim)
}
builtin_transplant_builtin_class(Exception, builtin_Exception)

class builtin_Fiber {
    static func current = builtin_currentfiber()
}

builtin_transplant_builtin_class(Fiber, builtin_Fiber)

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
                print(".exit        Exit from the REPL")
                print(".clear       Clear REPL history")
                print(".help        Show this help message")
                break
            }

            default {
                try {
                    const module = compile(input, "repl")
                    const fiber = spawn module()
                    $$ = await fiber
                } catch(e) {
                    $$ = e
                    print("Caught exception:")
                }

                print($$)
            }
        }

        input = prompt()
    }
}

return @"charly.boot"()