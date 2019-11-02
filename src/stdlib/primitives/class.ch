export = ->(Base) {
  return class Class extends Base {
    func @"<<"(o) {
      self(o)
    }
  }
}
