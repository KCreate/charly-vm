const system_clock_now = Charly.internals.get_method("Time::system_clock_now")
const steady_clock_now = Charly.internals.get_method("Time::steady_clock_now")

// Helper classes
class Timestamp {
  property ts
}

class Duration extends Timestamp {

}

export = {

  // Direct clock access
  system_clock_now, steady_clock_now,

  // Helper classes
  Timestamp, Duration,

  // Time manipulation methods
  add: ->(l, r) {

  },
  sub: ->(l, r) {

  },
  ceil: ->(l, r) {

  },
  floor: ->(l, r) {

  }
}
