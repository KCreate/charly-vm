- Maybe remove prototype property on classes

- Programmatic access to call / frame stack
  - A way to represent a machine frame in Charly.

- Changes to classes system
  - Member functions of classes should have a property which stores optional
    class information, such as the class to which this method belongs to,
    its super function and

- Remove call_dynamic from VM
  - Operator overloading can be implemented without it
  - Remove operator overload from operator methods
  - Return a specific value that signifies that no condition matched
    If that value is returned, perform the operator overload check
  - Add helper methods that simply perform an operation whichout doing any
    operator overload checking, defaulting to null

- Store timers and intervals in a min heap, based on their scheduled time
  - This way we don't have to search through all the scheduled events to find the next one
  - to execute

- Implement promises class

- Parse binary numbers
  -  8-bit numbers    0b00000000
  - 16-bit numbers    0b00000000_00000000
  - 32-bit numbers    0b00000000_00000000_00000000_00000000
  - 64-bit numbers are not allowed, since the bitwise operators operate on 32-bit numbers only
    anyway

- Expose parser and compilation infrastructure to charly code.

- Export keyword `export class Foo {}`

- Changes to import system, see feature-ideas/import-system.ch

- Changes to the class system
  - Primary classes can have their constructor auto-generated
  - Subclasses are required to have a hand-written constructor if new properties are defined
    - That constructor is required to contain a 'super(...)' call
  - 'super.foo' should call the foo method of the parent object
    - super is a keyword and cannot be used as a regular identifier
    - super can only be used inside class constructors and functions
      - Inside a constructor it will invoke the constructor of the parent class
        - There can only be one superconstructor call inside the constructor method
      - Inside a class function it will invoke the function defined by the parent class
        - If no such function exists, an exception is thrown

- New global class: Error
  - `throw new Error("something bad happened")`
  - Possible subclasses:
    - RangeError
    - ReferenceError
    - TypeError
  - Implement a global basic exception handler which just prints the message
    and a nice human readable stacktrace.
  - If the global exception handler is reached it should not crash the whole machine
    but only the currently running Fiber (see Fiber ideas below)
  - Syntax to only catch certain specific exceptions (i.e: `try {...} catch(CustomError e) {...}`)

- Argument spread operator
  - Collect all remaining arguments
    `
      func f(a, b, c...) {
        return [a, b, c]
      }

      f(1, 2, 3, 4, 5, 6) // => [1, 2, [3, 4, 5, 6]]
    `

- Call library destructors inside vm panic handler

- Spread operator
  - Different parsing styles
    - ...<exp>        Unpack array or object
    - <exp>..<exp>    Inclusive range e.g '1..3' would be Range{1, 2, 3}
    - <exp>...<exp>   Exclusive range e.g '1...3' would be Range{1, 2}
    - func foo(<ident>...)   Argument spread operator, see argument-spread.ch
  - Places where the spread operator can be used
    - Inside function calls 'foo(...args)'
    - Inside array literals '[...other_array]'
    - Inside object literals '{ ...other_obj }'

- Iterators
  - Types which can be iterated over should all have a common method to obtain
    such an iterator. The returned object will be of the class 'Iterator'.
  - Can be implemented using generators

- Ability to pause a thread of execution and resume it at a later time
  - Usecase #1: Waiting for a Timer to finish
  - `
      const handle = defer(->{
        print("waited for 1 second")
      }, 1000)

      handle.wait()

      print("this runs after the timer")
    `
  - Maybe implement a lightweight fiber system with a task scheduler?
    - Task scheduler should be configurable via charly code.
    - Different fibers should not run in parallel, but concurrently, meaning
      the task scheduler can pause a fiber and resume another without the programmer knowing.
    - Ability to block transfer to another fiber for some time
    - Ability to manually pass control to some other fiber
    - Is this actually a good idea? Is it a better idea to just follow through with the whole
      event queue system?
    - Ability to communicate between fibers, similar to how Go implements Channels
    - Mutexes, locks etc.

- Ability to flush stdout, stderr

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

- Improved Garbage Collector
  - Implement a fully fledged garbage collector that has the ability to crawl
    the call tree, inspect the machine stack and search for VALUE pointers (pointers
    pointing into the gc heap area)

- Fix runtime crash when executing the runtime profiler
  - A frame gets deallocated while the vm is still using it, resulting in the return instruction
    jumping to a nullpointer. This bug has to be related to some place not having the right checks
    in place.

- Implement `bound_self` for functions
