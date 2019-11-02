const __system_clock_now = Charly.internals.get_method("Time::system_clock_now")
const __steady_clock_now = Charly.internals.get_method("Time::steady_clock_now")
const __to_local = Charly.internals.get_method("Time::to_local")
const __to_utc = Charly.internals.get_method("Time::to_utc")
const __fmt = Charly.internals.get_method("Time::fmt")
const __fmtutc = Charly.internals.get_method("Time::fmtutc")
const __parse = Charly.internals.get_method("Time::parse")

// Helper classes
class Timestamp {
  property ts

  func constructor(o) {
    if (typeof o == "object") {
      @ts = o.ts
      return
    }
    @ts = o
  }

  func add(o) {
    Timestamp(@ts + o.ts)
  }
  func @"+"(o) = @add(o)

  func sub(o) {
    Timestamp(@ts - o.ts)
  }
  func @"-"(o) = @sub(o)

  func equals(o) {
    @ts == o.ts
  }
  func @"="(o) = @equals(o)

  func floor(d) {
    const rem = @ts % d.ts
    Timestamp(@ts - rem)
  }

  func ceil(d) {
    const rem = @ts % d.ts
    Timestamp(@ts + (d.ts - rem))
  }

  func to_s {
    @to_utc()
  }

  func to_local {
    __to_local(@ts)
  }

  func to_utc {
    __to_utc(@ts)
  }

  func fmt(format) {
    __fmt(@ts, format)
  }

  func fmtutc(format) {
    __fmtutc(@ts, format)
  }
}

class Duration extends Timestamp {};

export = {
  now: ->Timestamp(__system_clock_now()),
  now_steady: ->Timestamp(__steady_clock_now()),

  parse: ->(string, fmt) {
    Timestamp(__parse(string, fmt))
  },

  Timestamp, Duration
}
