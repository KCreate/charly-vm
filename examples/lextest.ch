const write = Charly.internals.get_method
func print(val) {
  write(val)
  write("\n")
}

//const Test = class {
  //property foo
  //property bar

  //static property foo
  //static property bar

  //func constructor() {}
  //func member() {}

  //static func staticfunc() {}
//}

print(class {})
