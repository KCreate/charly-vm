const Person = func (name, age) = {
  name,
  age,
  talk: ->{
    print("hello world")
  }
}

const leonard = Person("leonard", 17)
leonard.talk()
