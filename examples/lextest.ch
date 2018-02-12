let a = 1

if true {
  let a = 2
  ->{ print(a) }()
}

let b = 3

print(a)
print(b)
