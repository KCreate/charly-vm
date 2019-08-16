export = ->(Base) {
  return class Object extends Base {
    func times(cb) {
      let i = 0

      while i < self {
        cb(i)
        i += 1
      }

      null
    }
  }
}
