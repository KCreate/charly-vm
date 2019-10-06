export = ->(Base) {
  const _ = {
    insert:  Charly.internals.get_method("PrimitiveArray::insert"),
    remove:  Charly.internals.get_method("PrimitiveArray::remove"),
    reverse: Charly.internals.get_method("PrimitiveArray::reverse"),
    flatten: Charly.internals.get_method("PrimitiveArray::flatten"),
    index:   Charly.internals.get_method("PrimitiveArray::index"),
    rindex:  Charly.internals.get_method("PrimitiveArray::rindex"),
    range:   Charly.internals.get_method("PrimitiveArray::range")
  }

  return class Array extends Base {
    func copy {
      @map(->$0)
    }

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

    func reduce(cb, first) {
      let i = 0
      let sum = first

      while i < @length {
        sum = cb(sum, self[i], i, self)
        i += 1
      }

      sum
    }

    func sum(first) {
      self.reduce(->(l, c) l + c, first)
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

    func empty {
      !@length
    }

    func push(item) {
      self << item
      self
    }

    func pop {
      const item = @last()
      _.remove(@length - 1)
      item
    }

    func unshift(item) {
      _.insert(self, item, 0)
      self
    }

    func shift(item) {
      const item = @first()
      _.remove(0)
      item
    }

    func insert(index, item) {
      _.insert(self, index, item)
      self
    }

    func remove(index) {
      _.remove(self, index)
      self
    }

    func iterate(callback) {
      let i = 0

      func read() {
        let v = self[i]
        i += 1
        v
      }

      while i < @length {
        callback(read)
        if i == 0 break
      }

      self
    }

    func first {
      self[0]
    }

    func last {
      self[@length - 1]
    }

    func reverse {
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
