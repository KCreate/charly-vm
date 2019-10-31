export = ->(Base) {
  return class Function extends Base {
    func @"<<"(o) {
      self(o)
    }
  }
}
