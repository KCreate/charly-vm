export = {
  foo: ->(callback) {
    callback()
  },

  bar: ->{
    throw "exc from foo.ch"
  }
}
