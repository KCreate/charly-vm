class ContainerIteratorResult {
  property value
  property index
  property done

  func constructor(@value, @index, @done) = null
}

class Container {
  property data

  func constructor(@data) = null
  func each {
    let i = 0
    while i < data.length {
      yield ContainerIteratorResult(data[i], i, false)
      i += 1
    }
    ContainerIteratorResult(null, i, true)
  }
}

const nums = Container([1, 2, 3, 4, 5, 6])
const iterator = nums.each()

while iterator {
  const result = iterator()
  if !result.done {
    print(result.value)
  }
}
