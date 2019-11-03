export = ->(Base) {
  return class Boolean extends Base {

    /*
     * Returns the number representation of this boolean
     * */
    func to_n {
      self ? 1 : 0
    }
  }
}
