// Multi value index expression
//
// Desugared to:
//
// let data = [1, 2, 3]
// %tmp = data
// %id0 = 0
// %id1 = -1
// %id2 = 1
// %cpy = [null, null, null]
// %cpy[%id0] = %tmp[%id0]
// %cpy[%id1] = %tmp[%id1]
// %cpy[%id2] = %tmp[%id2]
// %cpy
let data = [1, 2, 3]
data[0, -1, 1] // => [1, 3, 2]
