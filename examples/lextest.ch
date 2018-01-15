const module = require("examples/test-require.ch")

print(module.read())
module.write("hello world")
print(module.read())

const leonard = module.someclass("leonard", 17)
leonard.method()

print(Object)


print(200  + 200)
