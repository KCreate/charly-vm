class Factor {
  property value
}

class Add {
  property left
  property right
}

class Multiply {
  property left
  property right
}

const x = Factor(20)
const l = Add(x, Factor(5))
const r = Add(x, Factor(-5))
const m = Multiply(l, r)

func binomial(t) {
  const ops = []

  ops << Factor(t.left.left.value * t.left.left.value)
  ops << Factor(t.left.left.value * t.right.right.value)
  ops << Factor(t.left.right.value * t.right.left.value)
  ops << Factor(t.left.right.value * t.right.right.value)

  ops
}

print(m)
print(binomial(m))
