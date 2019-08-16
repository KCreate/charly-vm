export = ->(Base) {
  return class Array extends Base {
    func each(cb) {
      let i = 0

      while i < @length {
        cb(self[i], i, self)
        i += 1
      }

      self
    }

    func map(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        new_array << cb(self[i], i, self)
        i += 1
      }

      new_array
    }

    func filter(cb) {
      let i = 0
      const new_array = []

      while i < @length {
        const value = self[i]
        const include = cb(value, i, self)
        if include new_array << value
        i += 1
      }

      new_array
    }

    func reverse() {
      let i = @length - 1
      const new_array = []

      while i >= 0 {
        new_array << self[i]
        i -= 1
      }

      new_array
    }

    func contains(search) {
      let i = 0

      while i < @length {
        const value = self[i]
        if (value == search) return true;
        i += 1;
      }

      false
    }
  }
}
