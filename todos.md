- Way to reconcile global variables with undefined symbol warnings at compile-time
  - Special syntax to access global variables?

- Hoist local variable declarations
  - `
      func foo { bar() } <- would generate a readglobal call
      func bar { ... }
    `

- Replace symbol function with CRC32
  - Allows for compile-time calculation of hashes which in turn allows us to use
    the switch statement for symbols.

- Add ARGV, ENV mappings

- Implement control access to objects
  - Mark a key as private
    - Can only be accessed via functions which are registered
      member functions of the klass of the object in question
  - Mark a key as constant
    - Key cannot be overwritten or deleted

- Changes to the class system
  - Constructors of subclasses that define new properties are required to contain a 'super(...)' call
  - No 'super.foo(...)' syntax
    - Functions can only call their own super function, not any arbitrary one
    - Special syntax for 'super.call(...)' to call with a custom self object and arguments
  - Hide prototype variable
    - Add internal method to add new method to prototype
    - Throw if the method already belongs to a class
    - Configure klass property of function

- Raw memory access methods
  - Create a library and internal methods that allow access to raw memory, via pointers
  - Load buffers of specific size
  - Some kinda DSL that allows the automatic mapping of memory to offset fields
    (basically recreate the C struct in Charly)

- Refactor codebase
  - Compiler, Parser stuff is a huge mess
  - VM should keep track of compiled blocks, source information etc.
    This is currently also a huge mess and needs to be cleaned up before things
    like sourcemaps can become a reality
  - ASTNodes should be passed around by shared_ptr's

- Instruction Pointer to filename & line number mapping for better stack traces
  - Foreach entry inside a block, map current bytecode offset to the starting linenumber of the entry

- Store timers and intervals in a min heap, based on their scheduled time
  - This way we don't have to search through all the scheduled events to find the next one
    to execute

- Promise class
  - Main abstraction class for all asynchronous methods in the standard library
  - Introduce await keyword to wait for a promise to finish

- Class function overloading based on argument size
  - Looks like a single function from the outside
    - Dispatch to correct function inside compiler-generated method
  - `
      class Math {
        rand()         = __get_random(0, 1)
        rand(max)      = __get_random(0, max)
        rand(min, max) = __get_random(min, max)
      }
    `
- Refactor unit tests
  - Organize into different categories
    - Basic VM functionality
    - Primitive classes standard methods
    - Standard libraries

- String interpolation
  - `
      // Parsing
      const name = "leonard"
      "Hello #{name}"               "Hello leonard"
      "Hello \#{name}"              "Hello #{name}"
      "Hello #name"                 "Hello #name"
      "Hello #{25 + 25}"            "Hello 50"

      // Tree-rewrite
      "Hello #{name}, how are you?"       "Hello " + (name).to_s() + ", how are you?"
      "Number: #{25 + 25}"                "Number: " + (25 + 25).to_s()
      "#{25 + 25}"                        (25 + 25).to_s()
    `
  - Lexer modes
    - http://www.oilshell.org/blog/2017/12/17.html
    - https://www.reddit.com/r/ProgrammingLanguages/comments/932372/how_to_implement_string_interpolation/

- Implement C signal handlers

- Ability to freeze objects
  - Simple status bits on every object
    - f1 = freeze_values        When set to 1, no values can be overwritten
    - f2 = freeze_keys          When set to 1, no keys can be added or removed

- Implement a notifier class which can resume certain threads once a condition is met
  - Can be used to implement the defer timer and interval handles in a cleaner way
  - Useful for async worker thread methods

- Parse binary numbers
  -  8-bit numbers    0b00000000
  - 16-bit numbers    0b00000000_00000000
  - 32-bit numbers    0b00000000_00000000_00000000_00000000
  - 64-bit numbers are not allowed, since the bitwise operators operate on 32-bit numbers only
    anyway

- Re-implement operator overloading in a good way

- Expose parser and compilation infrastructure to charly code.

- Changes to import system, see feature-ideas/import-system.ch

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
