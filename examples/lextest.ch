import "time"

let i = 0
let ts = time.now_highres()

let arr = [
  "1",
  "2",
  "3",
  "4"
]

let sum = 0

let iters = 100000
while i < iters {
  15.times(->arr.reverse())

  const now = time.now_highres()
  sum += (now - ts)
  ts = now

  i += 1
}
print(sum / iters)
