const l = import "../libraries/testlib.lib"

print(l.read())

l.add(50)
l.add(500)
l.add(-350)

print(l.read())
