//Charly.set_default_exception_handler(->(e) {
  //print("Caught an exception: " + e)
//})

//try {
  //defer(->{
    //throw "hello world"
  //})
//} catch(e) {
  //print(e)
//}

const buf = String.Buffer(2)

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

buf.reserve(256)

print(buf)
buf.write("hello world")

buf.reserve(128)

print(buf)
buf.write("hello world")

buf.reserve(4)

print(buf)
buf.write("hello world")

print(buf)
buf.write("hello world")

const result = buf.str()
print(result)
