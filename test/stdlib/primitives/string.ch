/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

export = ->(describe, it, assert) {
  it("lowercases a string", ->{
    assert("HELLO WORLD".lowercase(), "hello world")
    assert("TeSt 1234".lowercase(), "test 1234")
    assert("hello world".lowercase(), "hello world")
    assert("".lowercase(), "")
  })

  it("uppercases a string", ->{
    assert("hello world test".uppercase(), "HELLO WORLD TEST")
    assert("hello WORLD test".uppercase(), "HELLO WORLD TEST")
    assert("".uppercase(), "")
    assert("12345".uppercase(), "12345")
  })

  it("trims a string", ->{
    const base = "      \t\n\v\f\rhello world      \t\n\v\f\r"

    assert(base.ltrim(), "hello world      \t\n\v\f\r")
    assert(base.rtrim(), "      \t\n\v\f\rhello world")
    assert(base.trim(),  "hello world")
  })

  it("returns a substring", ->{
    const base = "hello world this is a test"

    assert(base.substring(6, 5), "world")
    assert(base.substring(0, 5), "hello")
    assert(base.substring(12, 14), "this is a test")
    assert(base.substring(-4), "test")
    assert(base.substring(-14), "this is a test")
    assert(base.substring(-14, 7), "this is")
  })

  it("reverses a string", ->{
    assert("".reverse(), "")
    assert("1234".reverse(), "4321")
    assert("hello world".reverse(), "dlrow olleh")
    assert(".*#".reverse(), "#*.")
    assert("  ".reverse(), "  ")
  })

  it("each gives every character in the string", ->{
    const base = "hello world"
    const chars = []
    base.each(->(c) chars << c)
    assert(chars, ["h", "e", "l", "l", "o", " ", "w", "o", "r", "l", "d"])
  })

  it("creates a new string via map", ->{
    const base = "10100101001"

    const result = base.map(->(c) {
      if c == "1" return "HI"
      "LO"
    })

    assert(result, "HILOHILOLOHILOHILOLOHI")
  })

  it("filters a string", ->{
    const base = "hello world this is a test"

    const result = base.filter(->(c) {
      if c == "a" return false
      if c == "e" return false
      if c == "i" return false
      if c == "o" return false
      if c == "u" return false
      if c == " " return false
      true
    })

    assert(result, "hllwrldthsstst")
  })

  it("splits a string into pieces", ->{
    assert("hello world this is a test".split(" "), ["hello", "world", "this", "is", "a", "test"])
    assert("hello".split(""), ["h", "e", "l", "l", "o"])
    assert("/foo/bar/baz".split("/"), ["", "foo", "bar", "baz"])
    assert("00001010001010".split("1"), ["0000", "0", "000", "0", "0"])
  })

  it("searches for a substring", ->{
    const base = "hello world this is a test hello world"

    assert(base.index("hello"), 0)
    assert(base.index("world"), 6)
    assert(base.index("hello world"), 0)
    assert(base.index("foooo"), -1)
    assert(base.index("this is a test"), 12)
    assert(base.index("hello", 6), 27)
    assert(base.index("this", 17), -1)

    assert(base.rindex("hello"), 27)
    assert(base.rindex("world"), 33)
    assert(base.rindex("hello world"), 27)
    assert(base.rindex("foooo"), -1)
    assert(base.rindex("this is a test"), 12)
  })

  it("checks if a string contains another string", ->{
    const base = "hello world this is a test"

    assert(base.contains("hello"))
    assert(base.contains("world"))
    assert(base.contains("this is"))
    assert(base.contains("world this"))
    assert(base.contains(" "))

    assert(!base.contains("foooo"))
    assert(!base.contains("1234"))
    assert(!base.contains("       "))
  })

  it("checks wether a string begins or ends with another string", ->{
    const base = "hello world this is a test"

    assert(base.begins_with("hello"))
    assert(base.begins_with("h"))
    assert(base.begins_with("hello world"))
    assert(base.begins_with("hello world this is a test"))

    assert(base.ends_with("test"))
    assert(base.ends_with("a test"))
    assert(base.ends_with("this is a test"))
    assert(base.ends_with("hello world this is a test"))
  })

  it("checks wether a string is a number", ->{
    assert("0".is_digit())
    assert("1".is_digit())
    assert("2".is_digit())
    assert("3".is_digit())
    assert("4".is_digit())
    assert("5".is_digit())
    assert("6".is_digit())
    assert("7".is_digit())
    assert("8".is_digit())
    assert("9".is_digit())

    assert(" ".is_digit(), false)
    assert("".is_digit(), false)
    assert("12".is_digit(), false)
    assert("1234".is_digit(), false)
    assert("h".is_digit(), false)
    assert("a".is_digit(), false)
    assert("b".is_digit(), false)
    assert("hello".is_digit(), false)
  })

  it("converts a string to a number", ->{
    assert("0".to_n(), 0)
    assert("5".to_n(), 5)
    assert("10".to_n(), 10)
    assert("100".to_n(), 100)

    assert("-1".to_n(), -1)
    assert("-5".to_n(), -5)
    assert("-100".to_n(), -100)

    assert("10.25".to_n(), 10.25)
    assert("100.2".to_n(), 100.2)
    assert("1.12345".to_n(), 1.12345)
    assert("1000.242".to_n(), 1000.242)
    assert("-25.2525".to_n(), -25.2525)

    assert("test".to_n(), NaN)
    assert("".to_n(), NaN)
    assert("25f25".to_n(), NaN)
  })

  it("returns the first / last character", ->{
    assert("".first(), null)
    assert("hello".first(), "h")

    assert("".last(), null)
    assert("hello".last(), "o")
  })
}
