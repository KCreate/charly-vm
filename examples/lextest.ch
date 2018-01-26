const CreateVM = ->(initial_state) {
  let state = initial_state
  return ->(opcode) {
    switch opcode {
      case "add" { state += 1 }
      case "sub" { state -= 1 }
      case "print" { print(state) }
    }
  }
}

const Opcodes = [
  "add",
  "add",
  "add",
  "add",
  "add",
  "sub",
  "print"
]

const VM = CreateVM(0)

Opcodes.each(->(op) VM(op))
