export = ->(Base) {
  return class Number extends Base {

    /*
     * Calls the callback *self* times, passing the current iteration count
     * as the first argument
     * */
    func times(cb) {
      let i = 0

      while i < self {
        cb(i)
        i += 1
      }

      null
    }

    /*
     * The same as times, but shorter
     * */
    func n(cb) = @times(cb)

    /*
     * Calls the callback with each number from *self* down to *num*
     * Both *self* and num are inclusive
     * */
    func downto(num, callback) {
      let i = self

      while i >= num {
        callback(i)
        i -= 1
      }

      self
    }

    /*
     * Calls the callback with each number from *self* up to *num*
     * Both *self* and num are inclusive
     * */
    func upto(num, callback) {
      let i = self

      while i <= num {
        callback(i)
        i += 1
      }

      self
    }

    /*
     * Returns the absolute value of this number
     * */
    func abs {
      if self < 0 return -self
      self
    }

    /*
     * Returns the sign of this number
     * Returns 0 if self is 0
     * */
    func sign {
      if self < 0 return -1
      if self > 0 return 1
      0
    }

    /*
     * Returns the bigger number
     * */
    func max(o) {
      if o > self return o
      self
    }

    /*
     * Returns the smaller number
     * */
    func min(o) {
      if o < self return o
      self
    }

    /*
     * Checks if self is close to *expected* with a given *delta*
     * */
    func close_to(expected, delta) {
      (self - expected).abs() <= delta
    }

    // Converters for different time scales
    func millisecond  = Charly.time.Duration << self
    func milliseconds = Charly.time.Duration << self
    func second       = Charly.time.Duration << self * 1000
    func seconds      = Charly.time.Duration << self * 1000
    func minute       = Charly.time.Duration << self * 1000 * 60
    func minutes      = Charly.time.Duration << self * 1000 * 60
    func hour         = Charly.time.Duration << self * 1000 * 60 * 60
    func hours        = Charly.time.Duration << self * 1000 * 60 * 60
    func day          = Charly.time.Duration << self * 1000 * 60 * 60 * 24
    func days         = Charly.time.Duration << self * 1000 * 60 * 60 * 24
    func week         = Charly.time.Duration << self * 1000 * 60 * 60 * 24 * 7
    func weeks        = Charly.time.Duration << self * 1000 * 60 * 60 * 24 * 7
    func year         = Charly.time.Duration << self * 1000 * 60 * 60 * 24 * 365
    func years        = Charly.time.Duration << self * 1000 * 60 * 60 * 24 * 365
  }
}
