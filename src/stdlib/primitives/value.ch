export = -> {
  return class Value {
    func tap(cb) {
      cb(self)
      self
    }

    func pipe() {
      const pipes = arguments

      pipes.each(->(p) {
        guard typeof p == "function" {
          throw ("pipe expected an array of functions, got " + typeof p)
        }

        p(self)
      })

      self
    }

    func transform() {
      const pipes = arguments

      let result = self

      pipes.each(->(p) {
        guard typeof p == "function" || typeof p == "cfunction" {
          throw ("pipe expected an array of functions, got " + typeof p)
        }

        result = p(result)
      })

      result
    }
  }
}
