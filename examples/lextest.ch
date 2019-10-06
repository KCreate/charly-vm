const arr = [1, 2, 3]

arr.insert(3, 20)
arr.insert(2, 100)
arr.insert(0, 200)

let sum = 0

sum = arr.sum(0)
print(arr)
print(sum)

arr.remove(4)
arr.remove(0)
arr.remove(1)

sum = arr.sum(0)

print(arr)
print(sum)
