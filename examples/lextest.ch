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

class A {
  property name

  func foo {
    print(@name)
  }
}

const a = A("leonard")
a.foo()
