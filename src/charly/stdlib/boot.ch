/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2026 Leonard Schütz
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
const builtin_getenv = @"charly.builtin.core.getenv"
const builtin_readfile = @"charly.builtin.core.readfile"
const builtin_str_to_num = @"charly.builtin.core.strtonum"

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

const builtin_string_index_of = @"charly.builtin.string.index_of"
const builtin_string_split = @"charly.builtin.string.split"
const builtin_string_substring = @"charly.builtin.string.substring"

const builtin_math_floor = @"charly.builtin.math.floor"
const builtin_math_abs = @"charly.builtin.math.abs"
const builtin_math_sqrt = @"charly.builtin.math.sqrt"
const builtin_math_cbrt = @"charly.builtin.math.cbrt"

func getenv(key) {
    assert key instanceof String
    return builtin_getenv(key)
}

func readfile(path) {
    assert path instanceof String
    return builtin_readfile(path)
}

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

    static func section(title, callback) {
        assert title instanceof String
        assert callback instanceof Function
        const sw = Stopwatch()
        defer {
            write(title)
            write(" ran for ")
            write(sw.check())
            write("ms")
            print()
        }
        return callback()
    }
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
    class builtin_Value {
        func apply(callback) {
            assert callback instanceof Function
            callback(self)
        }

        func also(callback) {
            assert callback instanceof Function
            callback(self)
            self
        }
    }

    class builtin_Instance {
        func constructor = self
    }

    class builtin_Number {
        func floor = builtin_math_floor(self)
        func abs = builtin_math_abs(self)
        func sqrt = builtin_math_sqrt(self)
        func cbrt = builtin_math_cbrt(self)

        func max(other) {
            assert other instanceof Number
            if other > self {
                return other
            }
            return self
        }

        func min(other) {
            assert other instanceof Number
            if other < self {
                return other
            }
            return self
        }

        func inBetween(a, b) {
            if b < a {
                const tmp = a
                a = b
                b = tmp
            }

            self >= a && self <= b
        }
    }

    class builtin_Int {
        func times(cb) {
            assert cb instanceof Function
            let i = 0
            while i < self {
                cb(i)
                i += 1
            }
            self
        }

        func parallelTimes(cb) {
            assert cb instanceof Function
            const tasks = List.create(self).map(->(e, i) spawn cb(i))
            tasks.each(->(task) await task)
            self
        }

        func map(cb) {
            assert cb instanceof Function
            const result = List.create(self)
            result.map(cb)
        }

        func parallelMap(cb) {
            assert cb instanceof Function
            const result = List.create(self)
            result.parallelMap(cb)
        }

        func upTo(other, callback) {
            assert other instanceof Number
            assert callback instanceof Function

            let i = self
            while i <= other {
                callback(i)
                i += 1
            }

            self
        }

        func downTo(other, callback) {
            assert other instanceof Number
            assert callback instanceof Function

            let i = self
            while i >= other {
                callback(i)
                i -= 1
            }

            self
        }

        func countTo(other, callback) {
            assert other instanceof Number
            assert callback instanceof Function

            if other < self {
                return other.countTo(self, callback)
            }

            self.upTo(other, callback)
        }

        func collectUpTo(other, callback = ->(n) n) {
            assert other instanceof Number
            assert callback instanceof Function

            const result = []
            @upTo(other, ->(...args) {
                result.push(callback(...args))
            })
            result
        }

        func collectDownTo(other, callback = ->(n) n) {
            assert other instanceof Number
            assert callback instanceof Function

            const result = []
            @downTo(other, ->(...args) {
                result.push(callback(...args))
            })
            result
        }
    }

    class builtin_String {
        func begins_with(other) {
            return @index_of(other) == 0
        }

        func index_of(search) {
            assert search instanceof String
            return builtin_string_index_of(self, search)
        }

        func chars() {
            return [...self]
        }

        func substring(start = 0, count = @length) {
            assert start instanceof Number
            assert count instanceof Number
            return builtin_string_substring(self, start, count)
        }

        func split(terminator = " ") {
            assert terminator instanceof String
            return builtin_string_split(self, terminator)
        }

        func lines {
            const lines = @split("\n")

            if lines.notEmpty() && lines.last() == "" {
                lines.pop()
            }

            return lines
        }

        func padRight(minimumLength, character = " ") {
            assert character instanceof String
            assert character.length == 1

            if @length >= minimumLength {
                return self
            }

            const remainingCharacters = minimumLength - @length
            const padString = character * remainingCharacters
            return self + padString
        }

        func padLeft(minimumLength, character = " ") {
            assert character instanceof String
            assert character.length == 1

            if @length >= minimumLength {
                return self
            }

            const remainingCharacters = minimumLength - @length
            const padString = character * remainingCharacters
            return padString + self
        }

        func to_number() {
            return builtin_str_to_num(self)
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
            return List.create_with(@length, ->(i) {
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

        func indices = List.create_with(@length, ->(i) i)

        func clear {
            if @length > 0 {
                @erase(0, @length)
            }
            self
        }

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

        func mapNotNull(cb) {
            return @map(cb).filter(->(n) n != null)
        }

        func parallelMap(cb) {
            return self.map(->(...args) {
                spawn cb(...args)
            }).map(->(fiber) {
                await fiber
            })
        }

        func flatten {
            const result = []

            @each(->(e) {
                if e instanceof List {
                    const c = e.flatten()
                    c.each(->(ce) result.push(ce))
                } else {
                    result.push(e)
                }
            })

            result
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

        func filterEmpty {
            @filter(->(e) e.length > 0)
        }

        func any(cb) {
            assert cb instanceof Function
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

                if cb(value, i, self) {
                    return true
                }

                i += 1
            }

            return false
        }

        func all(cb) {
            assert cb instanceof Function
            !@any(->(...args) !cb(...args))
        }

        func reverse {
            const result = List.create(@length)

            @each(->(e, i) {
                result[@length - i - 1] = e
            })

            result
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
            const length = @length

            if length <= 1 {
                return self
            }

            if compare_function == null {
                compare_function = ->(left, right) left <=> right
            }

            // secondary buffer, same size
            let buffer = self.sublist(0, length)

            // we alternate between these on each pass
            let src = self
            let dst = buffer

            let width = 1
            while (width < length) {
                let i = 0

                while (i < length) {
                    let left  = i
                    let mid   = i + width
                    let right = i + width * 2

                    if mid > length {
                        mid = length
                    }
                    if right > length {
                        right = length
                    }

                    // if there's no right run, just copy the tail
                    if mid >= right {
                        let t = left
                        while (t < right) {
                            dst[t] = src[t]
                            t += 1
                        }
                        i = right
                        continue
                    }

                    let a = left
                    let b = mid
                    let k = left

                    while (a < mid && b < right) {
                        if compare_function(src[a], src[b]) <= 0 {
                            dst[k] = src[a]
                            a += 1
                        } else {
                            dst[k] = src[b]
                            b += 1
                        }
                        k += 1
                    }

                    while (a < mid) {
                        dst[k] = src[a]
                        a += 1
                        k += 1
                    }

                    while (b < right) {
                        dst[k] = src[b]
                        b += 1
                        k += 1
                    }

                    i = right
                }

                // next pass: swap roles
                let tmp = src
                src = dst
                dst = tmp

                width = width * 2
            }

            // if final result ended up in buffer, copy back once
            if src != self {
                let i = 0
                while (i < length) {
                    self[i] = src[i]
                    i += 1
                }
            }

            self
        }

        func sublist(start, count = @length) {
            assert start instanceof Number
            assert count instanceof Number

            const result = []

            0.upTo(count - 1, ->(i) {
                const index = start + i
                if index >= 0 && index < @length {
                    result.push(self[index])
                }
            })

            result
        }

        func sum = @reduce(0, ->(p, c) p + c)

        func product = @reduce(1, ->(p, c) p * c)

        func join(separator = ", ", accessor = ->(e) e) {
            let result = ""
            @map(accessor).each(->(n, i) {
                if i == 0 {
                    result = "{n}"
                } else {
                    result = "{result}{separator}{n}"
                }
            })
            result
        }

        func findMax {
            if @empty() {
                return null
            }

            let largestValue = null
            let largestIndex = -1

            @each(->(e, i) {
                if i == 0 || e > largestValue {
                    largestValue = e
                    largestIndex = i
                }
            })

            return self[largestIndex]
        }

        func findMaxBy(callback) {
            assert callback instanceof Function
            if @empty() {
                return null
            }

            let largestValue = null
            let largestIndex = -1

            @each(->(e, i) {
                const value = callback(e, i)
                if i == 0 || value > largestValue {
                    largestValue = value
                    largestIndex = i
                }
            })

            return self[largestIndex]
        }

        func findMin {
            if @empty() {
                return null
            }

            let smallestValue = null
            let smallestIndex = -1

            @each(->(e, i) {
                if i == 0 || e < smallestValue {
                    smallestValue = e
                    smallestIndex = i
                }
            })

            return self[smallestIndex]
        }

        func findMinBy(callback) {
            assert callback instanceof Function
            if @empty() {
                return null
            }

            let smallestValue = null
            let smallestIndex = -1

            @each(->(e, i) {
                const value = callback(e, i)
                if i == 0 || value < smallestValue {
                    smallestValue = value
                    smallestIndex = i
                }
            })

            return self[smallestIndex]
        }

        func findBy(callback) {
            assert callback instanceof Function
            let i = 0
            while i < @length {
                const value = self[i]

                if callback(value, i, self) {
                    return i
                }

                i += 1
            }

            return null
        }

        func find(search) = @findBy(->(e) e == search)

        func empty = @length == 0
        func notEmpty = @length > 0

        func copy = [...self]

        func first {
            if @length > 0 {
                return self[0]
            }

            return null
        }

        func last {
            const length = @length
            if length > 0 {
                return self[length - 1]
            }

            return null
        }

        func dropFirst(n) = @sublist(n, @length)

        func dropLast(n) = @sublist(0, @length - n)

        func takeFirst(n) = @sublist(0, n)

        func takeLast(n) = @sublist(@length - n, n)

        func concat(other) {
            assert other instanceof List
            const result = self.copy()
            other.each(->(e) result.push(e))
            result
        }

        func bidirectionalPermutations = List.build(->(list) {
            const unidirectional = self.unidirectionalPermutations()

            let i = 0
            let limit = unidirectional.length
            while i < limit {
                const permutation = unidirectional[i]
                const (a, b) = permutation
                list.push(permutation)
                list.push((b, a))
                i += 1
            }
        })

        func unidirectionalPermutations = List.build(->(list) {
            let i = 0
            let j = 1

            if self.length < 2 {
                throw "Not enough values in the list to form permutations"
            }

            while i < self.length {
                while j < self.length {
                    const c1 = self[i]
                    const c2 = self[j]
                    list.push((c1, c2))
                    j += 1
                }
                i += 1
                j = i + 1
            }
        })

        func adjacentPairs {
            const result = []

            if self.length <= 1 {
                throw "Not enough values in the list to form pairs"
            }

            let i = 0
            let limit = self.length - 1
            while i < limit {
                result.push((self[i], self[i + 1]))
                i += 1
            }

            result
        }

        func inGroupsOf(n) {
            const groups = []

            let subgroup = []
            @each(->(e) {
                subgroup.push(e)
                if subgroup.length >= n {
                    groups.push(subgroup)
                    subgroup = []
                }
            })

            if subgroup.length > 0 {
                groups.push(subgroup)
            }

            groups
        }

        static func create(length, initial = null) = builtin_list_create(length, initial)
        static func create_with(length, callback) {
            const l = List.create(length)

            length.times(->(i) {
                l[i] = callback(i)
            })

            l
        }

        static func build(callback) {
            assert callback instanceof Function
            const list = []
            callback(list)
            list
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
    builtin_transplant_builtin_class(Number, builtin_Number)
    builtin_transplant_builtin_class(Int, builtin_Int)
    builtin_transplant_builtin_class(String, builtin_String)
    builtin_transplant_builtin_class(Function, builtin_Function)
    builtin_transplant_builtin_class(Exception, builtin_Exception)
    builtin_transplant_builtin_class(ImportException, builtin_ImportException)
    builtin_transplant_builtin_class(AssertionException, builtin_AssertionException)
    builtin_transplant_builtin_class(Fiber, builtin_Fiber)
    builtin_transplant_builtin_class(Future, builtin_Future)
}()
