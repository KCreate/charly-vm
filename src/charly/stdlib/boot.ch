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

const builtin_writevalue = @"charly.builtin.writevalue"
const builtin_readline = @"charly.builtin.readline"
const builtin_readfile = @"charly.builtin.readfile"
const builtin_compile = @"charly.builtin.compile"
const builtin_exit = @"charly.builtin.exit"

func write(string) = builtin_writevalue(string)

func echo(string) {
    write(string);
    write("\n");
}

func readline(prompt = "> ") {
    write(prompt);
    return builtin_readline()
}

func exit(status = 0) = builtin_exit(status)

func compile(source, name = "repl") = builtin_compile(source, name)

func readfile(name) = builtin_readfile(name)

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
        input = readline()
    }

    loop {
        switch input {
            case "" {
                break
            }

            case ".exit" {
                return
            }

            case ".help" {
                echo(".exit        Exit from the REPL")
                echo(".help        Show this help message")
                break
            }

            default {
                const module = compile(input, "repl")
                try {
                    const result = module()
                    write("< ")
                    echo(result)
                } catch(e) {
                    echo("Caught exception:")
                    echo(e)
                }
            }
        }

        input = readline()
    }
}

return @"charly.boot"()