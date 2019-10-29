import "math"

const register_worker_task = Charly.internals.get_method("register_worker_task");

let l = 100
let c = 0

0.upto(l, ->(num) {
  let cb = ->(result) {
    print(math.floor(c / l * 100) + "% finished")
    c += 1
  }
  register_worker_task(num, cb)
})

