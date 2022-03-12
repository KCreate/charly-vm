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

func times(n, cb) {
    let i = 0
    while i != n {
        cb(i)
        i += 1
    }
}

func sleep(n = 1) {
    const limit = 1000
    let i = 0
    let j = 0

    while j != n {
        while i != limit {
            i += 1
        }
        j += 1
        i = 0
    }
}

func map(size, cb) {
    const result = list(size)
    times(size, ->(i) {
        result[i] = cb(i)
    })
    return result
}

const job = spawn {
    return (1, 2, 3)
}

const result = await job

echo("result: {result}")



// Represents a basic thread in the charly runtime
// A thread can either run in task mode or generator mode
class Thread {


}

class Task extends Thread {

}

class Generator extends Thread {
    func next(argument = null)
}






class Container {
    property list

    // declaration:                         signature:          error:
    func foo()                              // foo()
    func foo(&block)                        // foo_()
    func foo(bar = null)                    // foo()            duplicate function signature
    func foo(bar = null, &block)            // foo_()           duplicate function signature
    func foo(bar, baz = null)               // foo(_)
    func foo(bar, baz = null, &block)       // foo_(_)
    func foo(bar, baz)                      // foo(_, _)
    func foo(bar, baz, &block)              // foo_(_, _)

    // optional parameters do not count towards function signature
    // functions that accept a callback block have a different signature
    // than functions that do not accept one
}

func maketask(context, function, args)
func makegenerator(context, function, args)
func taskjoin(task)
func taskyield(arg)

class Container {
    func +(other)   // binary + operator
    func ==(other)  // binary == operator
    func !()        // unary ! operator (not)
    func -()        // unary - operator

    func await()    // await operator

    func [](index)
    func []=(index, value)
}



class Generator {
    construct new(&function) {

    }
}

const counter = async {
    let i = 0
    loop {
        yield i
        i++
    }
}




// use-case:        concurrently request and load files from a server
//                  provided is a list of URLs
//                  result should be a list of strings

const urls = get_urls()

const requests = urls.map -> (url) spawn fetch(url)

const files = requests.map -> (req) await req

files.each -> (file) print(file)

urls.map -> spawn fetch($)
    .map -> await $
    .each -> print($)













