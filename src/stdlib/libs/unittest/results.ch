export = class ResultPrinter {
  display(results) {

    // Print another newline to avoid conflicting with
    // the previous visitor
    print("\n")

    const status = results.passed()

    if status {
      print("All tests have passed")
    } else {
      print("Some test suites have failed")

      const failed_tests = {}

      let index = 1
      results.deep_failed(->(nodes) {

        // Extract the title of the failed test case
        const title = nodes.filter(->$1 > 0)
                            .map(->$0.title)
                            .join(" ")
        const fnode = nodes.last()

        write(index + ") ")
        print(title)

        print("Assertion #" + (fnode.index + 1))

        // Check if the types match
        if typeof fnode.expected == typeof fnode.real {
          print("Expected value: " + fnode.expected.to_s())
          print("Actual value:   " + fnode.real.to_s())
        } else {
          print("Expected type: " + typeof fnode.expected)
          print("Actual type:   " + typeof fnode.real)
        }
        write("\n")

        index += 1
      })
    }
  }
}
