import math

10.times(->{
  100.times(->{
    write(["#", "*", ".", " "][math.rand(0, 4)])
  })

  write("\n")
})
