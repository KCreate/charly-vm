import "math"
import "time"

class Point {
  property x
  property y
}

class Bounds {
  property width
  property height
}

class View {
  property pos
  property bounds
}

class Grapher {
  property bounds
  property view

  func render(io, graphed_function) {
    const bounds = @bounds
    const view = @view

    // Render table header
    io << "# Grapher Output " << Charly.time.now().to_local() << " #" << "\n"

    // Setup the drawing area
    const out = Array.create(bounds.height + 1, ->Array.create(bounds.width + 1, " "))

    0.upto(bounds.width, ->(x) {
      const cordx = (x / bounds.width) * view.bounds.width + view.pos.x
      0.upto(bounds.height, ->(y) {
        const cordy = (y / bounds.height) * view.bounds.height + view.pos.y

        const v = graphed_function(cordx, cordy)
        if v out[bounds.height - y][x] = "X"
      })
    })

    out.each(->(row) {
      let buf = ""
      row.each(->(p) {
        buf += p
      })
      write(buf)
      write("\n")
    })

    return out
  }
}

Number.prototype.pad = ->(l, c) {

}

func heart(x, y) {
  let d = 0.005

  // (x^2 + y^2 - 1)^3 - x^2 y^3 = 0
  const r0 = ((x ** 2 + y ** 2 - 1) ** 3) - ((x ** 2) * (y ** 3))
  r0.close_to(0, d)
}

func g1(x, y) {
  const r0 = math.sin(math.sin(x))
  const r1 = math.cos(math.sin(x * y) + math.cos(x))
  r0.close_to(r1, 0.1)
}

const bounds = Bounds(170, 80)
const view = View(Point(0, 0), Bounds(2, 2))
const grapher = Grapher(bounds, view)
loop {
  const n = Charly.io.getn("")

  switch n {
    case 0 {
      grapher.view.bounds.width += 1
    }

    case 1 {
      grapher.view.bounds.width -= 1
    }

    case 2 {
      grapher.view.bounds.height += 1
    }

    case 3 {
      grapher.view.bounds.height -= 1
    }
  }

  const render_time = time.measure(->{
    const graph = grapher.render(write, ->(x, y) {
      g1(x, y)
    })
  })

  print("Render time: " + render_time.to_s())
}
