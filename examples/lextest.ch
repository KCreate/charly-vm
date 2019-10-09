const lib = import "../libraries/testlib.lib"

const result = 1.transform(
  lib.mul_10,

  lib.add_1,
  lib.add_1,
  lib.add_1,
  lib.add_1,
  lib.add_1,

  lib.div_10
)

print(result)
