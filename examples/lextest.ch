func double_each[args] {
  return args.map(->$0 * 2)
}

double_each(1, 2, 3)    // -> [2, 4, 6]
double_each([1, 2, 3])  // -> [2, 4, 6]
