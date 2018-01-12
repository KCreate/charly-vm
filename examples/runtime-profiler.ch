// The purpose of this file is to touch every instruction supported by the machine at least once
// It is used as the sample file when compiling the production binary using the
// profile guided optimization flags for clang
//
// Usage:
// make profiledproduction

let i = 0
let iterations = 100000

const thrown_exceptions = []
while (i < iterations) {
  let a = 10
  let b = 10
  let c = a + b

  let container = {
    nums: [1, 2, 3, 4],
    obj: {
      num: 0,
      method: func {
        @num += 1
      }
    }
  }

  container.nums << 5
  container.nums << 6

  container.obj.method()
  container.obj.method()
  container.obj.method()

  if (container.obj.num == 3) {
    container.obj.method()
  }

  unless (container.obj.num ! 3) {
    container.obj.method()
  }

  let someclass_repetitions = 0
  const someclass = class SomeClass extends class Base {
    property boye

    func method {
      someclass_repetitions += 1
    }
  } {
    func call {
      @method()
      @method()
      @method()
    }
  }

  const myobj = someclass()
  myobj.call()
  myobj.call()
  myobj.call()

  let correct = someclass_repetitions == 9

  func somefunc {}
  somefunc()
  somefunc()
  somefunc()
  somefunc()

  try {
    throw {
      message: "hello world"
    }
  } catch (e) {
    thrown_exceptions << e.message
  }

  try {} catch (e) {}

  const types = [typeof [1, 2], typeof 25, typeof "hello world"]

  const n1 = 25
  const n2 = 5
  const a1 = [1, 2, 3]
  const a2 = [1, 2, 3]
  const s1 = "hello world"
  const s2 = "hello world 2"
  const calculations = [
    n1 + n2,
    n1 - n2,
    n1 * n2,
    n1 / n2,
    n1 % n2,
    n1 ** n2,
    n1 == n2,
    n1 ! n2,
    n1 < n2,
    n1 > n2,
    n1 >= n2,
    n1 <= n2,
    n1 << n2,
    n1 >> n2,
    n1 & n2,
    n1 | n2,
    n1 ^ n2,
    +n1,
    -n1,
    !n1,
    ~n1,

    a1 + a2,

    s1 == s2,
    s1 < s2,
    s1 > s1,
    s1 <= s2,
    s1 >= s2,
    s1 ! s2
  ]

  const floatnums = [
    1.1,
    2.2,
    3.3,
    4.4,
    5.5,
    6.6,
    7.7,
    8.8,
    9.9
  ]

  func readarrayindex {
    const nums = [$0, $1, $2, $3, $4, $5]
  }

  i += 1
}
