export = ->(Base) {
  return class Generator extends Base {
    func @"<<"(o) {
      self(o)
    }
  }
}
