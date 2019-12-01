import "unittest"

const result = unittest(->(describe, it, assert) {

  describe("Arithmetic operations", ->{

    it("adds numbers", ->{
      const a = 2
      const b = 2
      const c = a + b

      assert(c, 4)
    })

    it("multiplies numbers", ->{
      const a = 2
      const b = 5
      const c = a * b

      assert(c, 14)
    })

  })

  describe("yeet", ->{

    describe("yeet 2", ->{
      it("does some random stuff", ->{
        assert(10, 50)
        assert(15, 50)
      })
    })

    it("does some random stuff", ->{
      assert(10, 50)
      assert(15, 50)
    })

  })

})

unittest.display_result(result)
