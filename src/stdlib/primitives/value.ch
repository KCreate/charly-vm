export = -> {
  return class Value {
    func tap(cb) {
      cb(self)
      self
    }
  }
}
