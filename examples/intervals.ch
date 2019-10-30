import "time"

let t = time.system_clock_now()
defer.interval(->(i) {
  const now = time.system_clock_now()
  print("milliseconds since last callback invocation: " + (now - t))
  t = now
}, 1000)
