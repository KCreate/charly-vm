let obj = { name: "leonard" }

try {
  defer(->{
    throw obj
  })
} catch(e) {
  print(e)
}

print("exited try statement")
