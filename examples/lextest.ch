const print = Charly.internals.get_method

const arr1 = [1, 2, 3]
const arr2 = [1, 2, 3]

arr1 << arr1
arr2 << arr2

print(arr1 == arr2)
