class Foo {
    property foo

    func constructor(x) {
        echo("inside Foo constructor with x: {x}")
    }

    func get1 {
        return 1
    }
}

class Bar extends Foo {
    property bar

    func constructor(x) {
        echo("inside Bar constructor with x: {x}")
    }

    func get2 {
        return 2
    }
}

const f = Foo(25)
const b = Bar(100)

writeline(f)
writeline(b)

writeline(typeof f)
writeline(typeof b)

writeline(typeof 25)
writeline(typeof 25.5)
writeline(typeof true)
writeline(typeof null)
writeline(typeof "hello")
writeline(typeof "hello world my name is leonard")
writeline(typeof (1, 2, 3))
writeline(typeof writeline)
writeline(typeof Foo)
writeline(typeof Int)
writeline(typeof builtin_writeline)
writeline(typeof @"charly.mainfiber")
