const print = Charly.internals.get_method

const create_exception = ->(message) ({
  type: "exception",
  message
})
const throw_thing = ->(thing) throw create_exception(thing)

try {
  throw_thing(25)
} catch(e) {
  print(e)
}
