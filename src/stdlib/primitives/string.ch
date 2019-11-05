const Buffer = import "libs/string/buffer.ch"

// String Methods
const __to_n                 = Charly.internals.get_method("String::to_n")

export = ->(Base) {
  return class String extends Base {
    static property Buffer = Buffer

    static property whitespace = [" ", "\b", "\f", "\n", "\r", "\v"]

    static property digits = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]

    static property lowercase = [ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
                                  "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
                                  "u", "v", "w", "x", "y", "z"]

    static property uppercase = [ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
                                  "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
                                  "U", "V", "W", "X", "Y", "Z"]

    /*
     * Returns a new string which is a substring
     * starting at *start* with a length of *offset*
     *
     * ```
     * "hello world".substring(5, 5) // => "world"
     * ```
     * */
    func substring(start) {
      const offset = arguments[1] || @length
      let index = start
      const end = start + offset
      let result = ""

      while index < end {
        if self[index] result += self[index]
        index += 1
      }

      result
    }

    /*
     * Checks if this string is a digit
     *
     * ```
     * "5".is_digit() // => true
     * "2".is_digit() // => true
     * "0".is_digit() // => true
     * "f".is_digit() // => false
     * ```
     * */
    func is_digit {
      const r = ({
        @"0": true,
        @"1": true,
        @"2": true,
        @"3": true,
        @"4": true,
        @"5": true,
        @"6": true,
        @"7": true,
        @"8": true,
        @"9": true
      })[self]
      r || false
    }

    /*
     * Convert this string into a number
     *
     * ```
     * "2562".to_n() // => 2562
     * "-2562".to_n() // => -2562
     * "25.5".to_n() // => 25.5
     * "-1.88".to_n() // => -1.88
     * "test".to_n() // => NaN
     * ```
     * */
    func to_n {
      if self.length == 0 return NaN
      if self == " " return NaN
      __to_n(self)
    }
  }
}
