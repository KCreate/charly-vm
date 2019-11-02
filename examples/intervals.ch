import "time"

let t = time.now()
defer.interval(->(i) {
  const now = time.now()

  50000.times(->{
    [1, 2, 3].reverse()
  })

  const duration = now - t
  print("ø: " + duration.to_s())
  t = now
}, 1500)

let d = 500.years() + 20.days() + 12.hours() + 34.minutes() + 33.seconds() + 2.milliseconds()
print(d)
