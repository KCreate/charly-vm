class Vec2 {
  property x
  property y

  func @"+"(o) {
    Vec2(@x + o.x, @y + o.y)
  }

  func to_s {
    "x: " + @x + " y: " + @y
  }
}

let v = Vec2(5, 0)

defer.interval(->{
  v += Vec2(0, -1)
  print(v)
}, 1000)
