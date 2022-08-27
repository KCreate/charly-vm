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

const builtin_currentfiber = @"charly.builtin.core.currentfiber"
const builtin_transplant_builtin_class = @"charly.builtin.core.transplantbuiltinclass"
const builtin_writevalue = @"charly.builtin.core.writevalue"
const builtin_currentworkingdirectory = @"charly.builtin.core.currentworkingdirectory"
const builtin_getstacktrace = @"charly.builtin.core.getstacktrace"
const builtin_disassemble = @"charly.builtin.core.disassemble"
const builtin_maketuple = @"charly.builtin.core.maketuple"
const builtin_exit = @"charly.builtin.core.exit"
const builtin_performgc = @"charly.builtin.core.performgc"
const builtin_compile = @"charly.builtin.core.compile"

const builtin_readline_prompt = @"charly.builtin.readline.prompt"
const builtin_readline_add_history = @"charly.builtin.readline.add_history"
const builtin_readline_clear_history = @"charly.builtin.readline.clear_history"

const builtin_future_create = @"charly.builtin.future.futurecreate"
const builtin_future_resolve = @"charly.builtin.future.futureresolve"
const builtin_future_reject = @"charly.builtin.future.futurereject"

func print(...args) = builtin_writevalue(...args, "\n")

func prompt(message = "", append_to_history = true) {
    const result = builtin_readline_prompt("{message}")

    if append_to_history && result instanceof String {
        builtin_readline_add_history(result)
    }

    result
}

func readline_add_history(message) = builtin_readline_add_history(message)

func readline_clear_history = builtin_readline_clear_history()

func exit(status = 0) = builtin_exit(status)

func performgc() = builtin_performgc()

func currentworkingdirectory = builtin_currentworkingdirectory()

func compile(source, name = "repl") = builtin_compile(source, name)

->{
    class builtin_Value {}

    class builtin_Instance {
        func constructor = self
    }

    class builtin_Int {
        func times(cb) {
            let i = 0
            while i != self {
                cb(i)
                i += 1
            }

            self
        }
    }

    class builtin_Tuple {
        func each(cb) {
            const size = @length
            let i = 0

            while i != size {
                cb(self[i], i, self)
                i += 1
            }

            self
        }

        func map(cb) {
            const rv = Tuple.make(@length)
            each(->(e, i) {
                rv[i] = cb(e, i, self)
            })
            rv
        }

        func reduce(cb, initial = null) {
            let sum = initial
            each(->(e, i, list) {
                sum = cb(sum, e, i, self)
            })
            sum
        }

        func sum = reduce(->(p, e) p + e, 0)

        static func make(length, initial = null) = builtin_maketuple(length, initial)

        static func make_with(length, cb) {
            const tuple = Tuple.make(length)
            tuple.each(->(_, i) {
                tuple[i] = cb(i)
            })
        }
    }

    class builtin_Function {
        func disassemble() {
            print("disassembly of {self}")
            builtin_disassemble(self)
        }
    }

    class builtin_Exception {
        func constructor(@message) {
            // TODO: trim correct numer of stack frames when called from subclass constructors
            @stack_trace = Exception.getstacktrace()
        }

        static func getstacktrace = builtin_getstacktrace()
    }

    class builtin_ImportException {
        func constructor(@message, @errors) {
            // TODO: trim correct numer of stack frames when called from subclass constructors
            @stack_trace = Exception.getstacktrace()
        }
    }

    class builtin_AssertionException {
        func constructor(@message, @left_hand_side, @right_hand_side = true, @operation_name = "==") {
            // TODO: trim correct numer of stack frames when called from subclass constructors
            @stack_trace = Exception.getstacktrace()
        }
    }

    class builtin_Fiber {
        static func current = builtin_currentfiber()
    }

    class builtin_Future {
        static func create(cb = null) {
            const future = builtin_future_create()

            if typeof cb == Function {
                spawn {
                    try {
                        const result = cb()
                        future.resolve(result)
                    } catch(e) {
                        future.reject(e)
                    }
                }
            }

            return future
        }

        func resolve(value = null) = builtin_future_resolve(self, value)

        func reject(value = Exception("Future rejected")) = builtin_future_reject(self, value)
    }

    class builtin_Class {
        func new(...args) = self(...args)
    }

    builtin_transplant_builtin_class(Value, builtin_Value)
    builtin_transplant_builtin_class(Instance, builtin_Instance)
    builtin_transplant_builtin_class(Class, builtin_Class)
    builtin_transplant_builtin_class(Tuple, builtin_Tuple)
    builtin_transplant_builtin_class(Int, builtin_Int)
    builtin_transplant_builtin_class(Function, builtin_Function)
    builtin_transplant_builtin_class(Exception, builtin_Exception)
    builtin_transplant_builtin_class(ImportException, builtin_ImportException)
    builtin_transplant_builtin_class(AssertionException, builtin_AssertionException)
    builtin_transplant_builtin_class(Fiber, builtin_Fiber)
    builtin_transplant_builtin_class(Future, builtin_Future)
}()
