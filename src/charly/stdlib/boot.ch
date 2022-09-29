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
const builtin_getbacktrace = @"charly.builtin.core.getbacktrace"
const builtin_disassemble = @"charly.builtin.core.disassemble"
const builtin_createtuple = @"charly.builtin.core.createtuple"
const builtin_createtuplewith = @"charly.builtin.core.createtuplewith"
const builtin_exit = @"charly.builtin.core.exit"
const builtin_getsteadytimestamp = @"charly.builtin.core.getsteadytimestamp"
const builtin_getsteadytimestampmicro = @"charly.builtin.core.getsteadytimestampmicro"
const builtin_timerfibercreate = @"charly.builtin.timer.fibercreate"
const builtin_timersleep = @"charly.builtin.timer.sleep"
const builtin_timercancel = @"charly.builtin.timer.cancel"
const builtin_compile = @"charly.builtin.core.compile"

const builtin_readline_prompt = @"charly.builtin.readline.prompt"
const builtin_readline_add_history = @"charly.builtin.readline.add_history"
const builtin_readline_clear_history = @"charly.builtin.readline.clear_history"

const builtin_future_create = @"charly.builtin.future.create"
const builtin_future_resolve = @"charly.builtin.future.resolve"
const builtin_future_reject = @"charly.builtin.future.reject"

const builtin_list_create = @"charly.builtin.list.create"
const builtin_list_insert = @"charly.builtin.list.insert"
const builtin_list_erase = @"charly.builtin.list.erase"
const builtin_list_push = @"charly.builtin.list.push"
const builtin_list_pop = @"charly.builtin.list.pop"

func write(...args) = builtin_writevalue(...args)

func print(...args) = write(...args, "\n")

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

func currentworkingdirectory = builtin_currentworkingdirectory()

func compile(source, name = "repl") = builtin_compile(source, name)

func timestamp = builtin_getsteadytimestamp()

func timestamp_micro = builtin_getsteadytimestampmicro()

func sleep(delay) {
    assert delay instanceof Number
    builtin_timersleep(delay)
}

class Stopwatch {
    private property start = timestamp_micro()
    func check = (timestamp_micro() - start) / 1000
}

class Timer {
    private property id
    private property callback
    property result = Future.create()

    func constructor(delay, @callback = ->{}, ...args) {
        assert delay instanceof Number
        assert callback instanceof Function

        @id = builtin_timerfibercreate(delay, func handle(args) {
            result.resolve_with(->callback(...args))
        }, self, (args,))
    }

    func cancel {
        builtin_timercancel(id)
        result.reject("Timer cancelled")
    }
}

->{
    class builtin_Value {}

    class builtin_Instance {
        func constructor = self
    }

    class builtin_Int {
        func times(cb) {
            let i = 0
            while i < self {
                cb(i)
                i += 1
            }

            self
        }

        func map(cb) {
            List.create_with(self, cb)
        }
    }

    class builtin_String {
        func begins_with(other) {
            if other.length > @length {
                return false
            }

            if other.length == @length {
                return self == other
            }

            let i = 0
            while i < other.length {
                if self[i] != other[i] {
                    return false
                }

                i += 1
            }

            true
        }
    }

    class builtin_Tuple {
        func each(cb) {
            const size = @length
            let i = 0

            while i < size {
                cb(self[i], i, self)
                i += 1
            }

            self
        }

        func map(cb) {
            return Tuple.create_with(@length, ->(i) {
                cb(self[i], i, self)
            })
        }

        func filter(cb) {
            const new = []

            @each(->(e, i, t) {
                if cb(e, i, t) {
                    new.push(e)
                }
            })

            (...new)
        }

        func reduce(sum, cb) {
            each(->(e, i, list) {
                sum = cb(sum, e, i, self)
            })
            sum
        }

        func contains(value) {
            let i = 0
            while i < @length {
                if self[i] == value {
                    return true
                }
                i += 1
            }
            false
        }

        func empty = @length == 0

        static func create(length, initial) = builtin_createtuple(length, initial)
        static func create_with(length, cb) = builtin_createtuplewith(length, cb)
    }

    class builtin_List {
        func insert(index, value) = builtin_list_insert(self, index, value)
        func erase(start, count = 1) = builtin_list_erase(self, start, count)
        func push(value) = builtin_list_push(self, value)
        func pop() = builtin_list_pop(self)

        func each(cb) {
            const length = @length

            let i = 0
            while i < length {
                if @length != length {
                    throw "List size changed during iteration"
                }

                let value
                try value = self[i] catch {
                    throw "List size changed during iteration"
                }

                cb(value, i, self)

                i += 1
            }

            self
        }

        func map(cb) {
            const new_list = []
            const length = @length

            @each(->(...args) {
                new_list.push(cb(...args))
            })

            if new_list.length != length {
                throw "ConcurrencyError: List size changed"
            }

            new_list
        }

        func filter(cb) {
            const new = []

            @each(->(e, i, l) {
                if cb(e, i, l) {
                    new.push(e)
                }
            })

            new
        }

        func reduce(sum, cb) {
            each(->(e, i, list) {
                sum = cb(sum, e, i, self)
            })
            sum
        }

        func contains(value) {
            let i = 0
            let length = @length

            while i < length {
                if @length != length {
                    throw "List size changed during iteration"
                }

                let tmp
                try tmp = self[i] catch {
                    throw "List size changed during iteration"
                }

                if tmp == value {
                    return true
                }

                i += 1
            }

            false
        }

        func sort(compare_function = null) {
            let length = @length
            let i = 0
            let had_swaps = true

            if length <= 1 {
                return self
            }

            if compare_function == null {
                compare_function = ->(left, right) left <=> right
            }

            while (had_swaps) {
                i = 0
                had_swaps = false

                if @length != length {
                    throw "List size changed during sort"
                }

                @each(->(value, index) {
                    if index < length - 1 {
                        const next_value = self[index + 1]
                        if compare_function(value, next_value) > 0 {
                            self[index] = next_value
                            self[index + 1] = value
                            had_swaps = true
                        }
                    }
                })
            }

            self
        }

        func empty = @length == 0

        func copy = [...self]

        static func create(length, initial = null) = builtin_list_create(length, initial)
        static func create_with(length, callback) {
            const l = List.create(length)

            length.times(->(i) {
                l[i] = callback(i)
            })

            l
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
            @backtrace = Exception.getbacktrace()
        }

        static func getbacktrace = builtin_getbacktrace()
    }

    class builtin_ImportException {}

    class builtin_AssertionException {}

    class builtin_Fiber {
        static func current = builtin_currentfiber()
    }

    class builtin_Future {
        static func create(cb = null) {
            const future = builtin_future_create()

            if cb instanceof Function {
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

        func resolve_with(callback) {
            try resolve(callback()) catch reject(error)
            self
        }

        func reject(value) {
            try {
                throw value
            } catch(e) {
                builtin_future_reject(self, e)
            }
        }
    }

    class builtin_Class {
        func new(...args) = self(...args)
    }

    builtin_transplant_builtin_class(Value, builtin_Value)
    builtin_transplant_builtin_class(Instance, builtin_Instance)
    builtin_transplant_builtin_class(Class, builtin_Class)
    builtin_transplant_builtin_class(Tuple, builtin_Tuple)
    builtin_transplant_builtin_class(List, builtin_List)
    builtin_transplant_builtin_class(Int, builtin_Int)
    builtin_transplant_builtin_class(String, builtin_String)
    builtin_transplant_builtin_class(Function, builtin_Function)
    builtin_transplant_builtin_class(Exception, builtin_Exception)
    builtin_transplant_builtin_class(ImportException, builtin_ImportException)
    builtin_transplant_builtin_class(AssertionException, builtin_AssertionException)
    builtin_transplant_builtin_class(Fiber, builtin_Fiber)
    builtin_transplant_builtin_class(Future, builtin_Future)
}()
