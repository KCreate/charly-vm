- Reduce instruction argument size
  - We do not need a 32bit integer to represent argc, lvarcount etc.
    If you have 4.2 billion arguments you have bigger problems and need therapy.
  - Stuff like 'argc', 'minimum_argc' can be represented using just 8 bits. I seriously
    doubt any good code will have a function with over 256 arguments.
  - 'lvarcount' should be represented by a 16bit int. It does not seem impossible to me for code
    to reach the 256 local variable limit, so to be future-proof 16 bits should be used.
  - Also document existing argument sizes in the opcode.h file

- Do not generate a default constructor for classes without any properties

- Implement default values for class properties

- Implement object & array destructuring assignment
  - `
      const obj = { name: "leonard", age: 20 }
      const { name, age } = obj
      name // => "leonard"
      age // => 20

      const nums = [1, 2, 3]
      const [a, b, c] = nums
      a // => 1
      b // => 2
      c // => 3
    `

- Implement globals
  - Remove compiler warning on unknown symbols, look them up at runtime
    in the globals table

- Match statements
  - The local variable allocator has been finished and this allows for the match
    syntax to be implemented now (see feature-ideas/match-statement.ch for proposal)

- Ability to pause a thread of execution and resume it at a later time
  - Usecase #1: Waiting for a Timer to finish
  - `
      const handle = defer(->{
        print("waited for 1 second")
      }, 1000)

      handle.wait()

      print("this runs after the timer")
    `

- Implement magic constants
  - __DIR__
  - __FILE__
  - __LINE__

- Color handling methods

- Rename `lstrip` and `rstrip` methods to some other more correct name
  - These methods should remove whitespace characters from the left or right side of a string
  - Add a `strip` method, which is equivalent to calling `rstrip(lstrip(<string>))`

- Instruction Pointer to filename & line number mapping for better stack traces
  - Create utility functions that return various location information of the call chain
    This is needed to completet the import system as some helper functions inside the prelude
    need to know from which file an import was requested.

- do panic if some vm methods are called not from the main thread

- locale handling?
  - localization framework
  - time methods need localisation

- calendar time operations
  - add a day, add a year, add 20 years
    - should automatically calculate leap days and stuff like that

- fix exceptions on deferred functions
  - declare a global exception handler in the charly object that gets
    called whenever there is no exception handler declared
      - also useful for thrown that are running in the top frame
        as there is no exception handler there too and it always results
        in some ugly looking error messages

- Improved Garbage Collector
  - Implement a fully fledged garbage collector that has the ability to crawl
    the call tree, inspect the machine stack and search for VALUE pointers (pointers
    pointing into the gc heap area)

- Rewrite the class system
  - `super` method needs to be supported
  - Multiple inheritance
  - Default values for member properties

- Fix runtime crash when executing the runtime profiler
  - A frame gets deallocated while the vm is still using it, resulting in the return instruction
    jumping to a nullpointer. This bug has to be related to some place not having the right checks
    in place.

- Implement `bound_self` for functions
