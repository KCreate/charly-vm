/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

const __system_clock_now = @"charly.stdlib.time.system_clock_now"
const __steady_clock_now = @"charly.stdlib.time.steady_clock_now"
const __highres_now      = @"charly.stdlib.time.highres_now"
const __to_local         = @"charly.stdlib.time.to_local"
const __to_utc           = @"charly.stdlib.time.to_utc"
const __fmt              = @"charly.stdlib.time.fmt"
const __fmtutc           = @"charly.stdlib.time.fmtutc"
const __parse            = @"charly.stdlib.time.parse"

// Helper classes
class Duration {
  property ms

  in_nanoseconds  = @ms * 1000000
  in_microseconds = @ms * 1000
  in_milliseconds = @ms
  in_seconds      = @ms / 1000
  in_minutes      = @ms / (1000 * 60)
  in_hours        = @ms / (1000 * 60 * 60)
  in_days         = @ms / (1000 * 60 * 60 * 24)
  in_weeks        = @ms / (1000 * 60 * 60 * 24 * 7)
  in_years        = @ms / (1000 * 60 * 60 * 24 * 365)

  add(o) = new Duration(@ms + o.ms)
  sub(o) = new Duration(@ms - o.ms)
  mul(n) = new Duration(@ms * n)
  div(n) = new Duration(@ms / n)
  mod(o) = new Duration(@ms % o.ms)

  equals(o)        = @ms == o.ms
  less_than(o)     = @ms < o.ms
  greater_than(o)  = @ms > o.ms
  less_equal(o)    = @ms <= o.ms
  greater_equal(o) = @ms >= o.ms

  components {
    const obj = {
      years: 0,
      days: 0,
      hours: 0,
      minutes: 0,
      seconds: 0,
      milliseconds: 0,
      microseconds: 0
    }

    let remainder = self.ms
    while remainder >= 1.nanosecond().ms {
      if remainder >= 1.year().ms {
        let c = Math.floor(remainder / 1.year().ms)
        obj.years = c
        remainder = remainder % 1.year().ms
      } else if remainder >= 1.day().ms {
        let c = Math.floor(remainder / 1.day().ms)
        obj.days = c
        remainder = remainder % 1.day().ms
      } else if remainder >= 1.hour().ms {
        let c = Math.floor(remainder / 1.hour().ms)
        obj.hours = c
        remainder = remainder % 1.hour().ms
      } else if remainder >= 1.minute().ms {
        let c = Math.floor(remainder / 1.minute().ms)
        obj.minutes = c
        remainder = remainder % 1.minute().ms
      } else if remainder >= 1.second().ms {
        let c = Math.floor(remainder / 1.second().ms)
        obj.seconds = c
        remainder = remainder % 1.second().ms
      } else if remainder >= 1.millisecond().ms {
        let c = Math.floor(remainder / 1.millisecond().ms)
        obj.milliseconds = c
        remainder = remainder % 1.millisecond().ms
      } else if remainder >= 1.microsecond().ms {
        let c = Math.floor(remainder / 1.microsecond().ms)
        obj.microseconds = c
        remainder = remainder % 1.microsecond().ms
      } else if remainder >= 1.nanosecond().ms {
        let c = Math.floor(remainder / 1.nanosecond().ms)
        obj.nanoseconds = c
        remainder = remainder % 1.nanosecond().ms
      }
    }

    obj
  }

  format() {
    const buf = new StringBuffer()

    const components = @components()

    if components.years > 0 {
      const c = components.years
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "years" : "year"))
    }
    if components.days > 0 {
      const c = components.days
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "days" : "day"))
    }
    if components.hours > 0 {
      const c = components.hours
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "hours" : "hour"))
    }
    if components.seconds > 0 {
      const c = components.seconds
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "seconds" : "second"))
    }
    if components.minutes > 0 {
      const c = components.minutes
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "minutes" : "minute"))
    }
    if components.seconds > 0 {
      const c = components.seconds
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "seconds" : "second"))
    }
    if components.milliseconds > 0 {
      const c = components.milliseconds
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "milliseconds" : "millisecond"))
    }
    if components.microseconds > 0 {
      const c = components.microseconds
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "microseconds" : "microsecond"))
    }
    if components.nanoseconds > 0 {
      const c = components.nanoseconds
      if buf.get_offset() buf.write(" ")
      buf.write(c + " " + (c > 1 ? "nanoseconds" : "nanosecond"))
    }

    buf.to_s()
  }

  to_s {
    @format()
  }

  to_n {
    @ms
  }
}

class Timestamp {
  property ms

  constructor(o) {
    if typeof o == "object" { @ms = o.ms }
    else                    { @ms = o }
  }

  add(o) = new Timestamp(@ms + o.ms)
  sub(o) {
    if o.klass == Duration return new Timestamp(@ms - o.ms)
    new Duration(@ms - o.ms)
  }

  equals(o)        = @ms == o.ms
  less_than(o)     = @ms < o.ms
  greater_than(o)  = @ms > o.ms
  less_equal(o)    = @ms <= o.ms
  greater_equal(o) = @ms >= o.ms

  floor(d) {
    const rem = @ms % d.ms
    new Timestamp(@ms - rem)
  }

  ceil(d) {
    const rem = @ms % d.ms
    new Timestamp(@ms + (d.ms - rem))
  }

  to_s {
    @to_utc()
  }

  to_n {
    @ms
  }

  to_local {
    __to_local(@ms)
  }

  to_utc {
    __to_utc(@ms)
  }

  fmt(format) {
    __fmt(@ms, format)
  }

  fmtutc(format) {
    __fmtutc(@ms, format)
  }
}

class Time {
  constructor {
    throw "Cannot initialize an instance of the Time class"
  }

  static now {
    new Timestamp(__system_clock_now())
  }

  static now_steady {
    new Timestamp(__steady_clock_now())
  }

  static now_highres {
    new Timestamp(__highres_now() / 1000000)
  }

  static parse(string, fmt) {
    new Timestamp(__parse(string, fmt))
  }

  static measure(cb) {
    const begin = Time.now_highres()
    cb()
    Time.now_highres().sub(begin)
  }

  static property Timestamp = Timestamp
  static property Duration = Duration
}

export = Time
