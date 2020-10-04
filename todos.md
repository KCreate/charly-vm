# Todos

- VM Refactor
  - Implementation timeline
    - Class interface to heap types
      - How are values allocated and created via the GC?
      - Rename Basic -> Header
        - Make properties protected
      - Start by replacing all accesses to the Object type with class methods
        - Learn from this and see if we have to change anything with our design
        - Is the Container type actually a good idea?
      - Operations such as eq, lt can also be moved out of the VM into the value classes themselves
      - Replace std::mutex with something more efficient (smaller and faster??)
        - This looks interesting: https://webkit.org/blog/6161/locking-in-webkit/
    - Fixed amount of native worker threads, waiting for jobs via some job queue
    - Refactor worker thread result return system
    - ManagedContext ability to mark cells as immortal
    - ManagedContext ability to reserve cells in advance
    - Some global values can be stored as atomics, no mutex needed
    - C methods should receive the following arguments
      - The ID of the fiber which created their task
      - A reference/pointer to the coordinator object
      - A reference to a managed context, which handles the allocations inside that worker function
      - The "self" value
      - The arguments passed to the function from charly
    - Refactor exceptions
      - How do native methods throw exceptions?
      - How are exceptions handled inside regular charly code?
    - Refactor thread system to use the states and ability to wait for certain signals
      - GC goes into its own thread
      - Fibers create a Fiber heap object that mirrors the real fiber

  - What threads are active in our runtime (N = logical CPU cores)
    - 1x Coordinator thread (main thread)   | Timers, tickers, task queue, fiber queue
    - 1x Async networking thread            | Epoll, kqueue
    - Nx Fiber executor threads             | Executes fibers
    - Nx Native code worker threads         | Executes async C code

  - ManagedContext doesn't have to keep a temporary list
    - After allocating the cell, it is marked as `immortal`.
    - This tells the garbage collector to never deallocate it, even if there
      are no references to it.
    - Once the ManagedContext is closed, all the allocates values get their `immortal` bit
      set to false, thus allowing them to be collected again.

  - Fiber executors, worker threads can be in certain modes
    - The coordinator can broadcast signals to all its threads, notifying them
      of certain events in the VM
    - A child thread can check what state the coordinator currently is in and can
      wait for a certain signal based on that state. The check and wait should be atomic.
    - The coordinator is a state machine which gets status updates and wait requests from its child
      threads.
    - Available states:
      - EXEC_CHARLY
      - EXEC_NATIVE
      - EXEC_NATIVE_PROTECTED
      - EXEC_GC
      - IDLE
    - Transitions between these states sometimes are dependant on outside state
      - For example, a thread executing native code which wants to switch to a protected section
        has to check if currently a GC is running. If there is it has to wait for the GC to finish
      - A thread wanting to execute charly code has to wait for the GC to finish
      - A thread wanting to execute native code doesn't have to wait for the GC to finish
        as native code sections are not allowed to interact with anything that the GC might interrupt
      - If the GC thread wants to perform a collection and some other thread is currently inside a
        protected section, then the GC will have to wait for all protected sections to finish
        - During this time, other threads will be waiting for the GC to finish
        - A protected section could therefore stall the GC and thus the entire VM if it takes too long
        - Protected sections shouldn't contain any big operations and should simply register the
          result values with the coordinator or VM and remove their immortal status.
      - If there is a thread that is currently waiting for all protected sections to end,
        any thread wanting to switch from native to native protected has to wait for that other task
        to finish as to not starve the program.

    - Scenarios
      - Fiber 1 ran out of memory, requests a GC. Fiber 2-4 pause and wait for GC to complete.
        Fiber 6-8 and Native 4 are executing protected sections. The GC is waiting for these sections
        to complete before starting its work. Native 2 and 3 are waiting for the GC to complete to
        enter their protected sections.
        ```
        GC       - ACTIVE                 (WAIT_FOR_PROTECTED -> ACTIVE)

        Fiber  1 - EXEC_CHARLY            (NEEDS_GC    -> EXEC_CHARLY)
        Fiber  2 - EXEC_CHARLY            (WAIT_FOR_GC -> EXEC_CHARLY)
        Fiber  3 - EXEC_NATIVE            (WAIT_FOR_GC -> EXEC_NATIVE_PROTECTED)
        Fiber  4 - EXEC_NATIVE            (WAIT_FOR_GC -> EXEC_CHARLY)
        Fiber  5 - IDLE
        Fiber  6 - EXEC_NATIVE_PROTECTED
        Fiber  7 - EXEC_NATIVE_PROTECTED
        Fiber  8 - EXEC_NATIVE_PROTECTED

        Native 1 - EXEC_NATIVE
        Native 2 - EXEC_NATIVE            (WAIT_FOR_GC -> EXEC_NATIVE_PROTECTED)
        Native 3 - EXEC_NATIVE            (WAIT_FOR_GC -> EXEC_NATIVE_PROTECTED)
        Native 4 - EXEC_NATIVE_PROTECTED
        ```

  - Elaborate on the ideas of the ManagedContext
    - We should be able to reserve some amount of cells via this interface
    - It manages reserved cells or puts the current fiber into a GC requesting mode
    - Also contains GC synchronisation methods, either waiting for a collection to finish or
      making sure that no collection can start while in this section

  - Introduce some mechanism by which instructions and worker threads can "reserve" memory cells in the GC.
    - That way we make sure that collections happen at well-defined points in the code and not inside
      random allocations.
    - Allocations should never trigger a GC. GC should always be triggered before an allocation might
      trigger one.

  - A GC collection happens once one fiber requests one. It then waits for all the other fibers
    to finish their current instruction / native function call.
    - Technically other fibers / native functions do not have to completely finish. If they want to do
      another allocation, they just pause on that allocation and we somehow have to tell the
      requesting thread that they are now paused.
    - The GC thread only has to make sure that all fiber and worker threads have entered some well-known
      region of code.
      - For fiber threads executing regular charly bytecodes, this means simply the begin
        of an instruction. The opcode handles reserve enough cells to fully execute themselves, to make sure
        they can make it to the next instruction without causing a GC.
      - For fibers running native C code or worker threads currently in some long running C section,
        this means these parts of the code have to be in some specific mode, which tells the GC thread
        that they won't interfere with anything that the GC is up to. This means being able to
        remove these threads from the GCs pause-list. If these threads finish their C business, they have to
        wait for the GC to finish its job before they can continue. this might cause a native thread to be
        blocked while the GC is running. if a worker task knows ahead of time how many memory cells it
        will need, it can reserve these, making sure it won't block at runtime.
        - This could be a tuneable parameter, "how many tasks should memory be pre-reserved for"

- Enforce max string length of 2^32 (uint32_t as length field)
- Enforce max array length of 2^32 (uint32_t as length field)

- Remove panic system from the VM, turn into asserts

- Move some `charly_` methods into the heap value classes themselves
  - Example: `charly_find_super_method` should just be a method on the Class class

- Path library
  - Write unit tests
  - Implement methods

- Write more unit tests
  - Primitives
    - Array
    - Boolean
    - Class
    - Function
    - Null
    - Number
    - Object
    - Value
  - Math module
  - Time module

- Fix worker threads a1, a2, a3, a4 mess

- Refactor stack storage
  - Allocate with initial size of 8kb / fiber
  - Total allowed stack size should 1024 kilobytes (or 1 megabyte)
  - Runtime access to stacksize
    - Charly.Stack.capacity():   Return current total capacity of stack
    - Charly.Stack.size():       Return remaining amount of VALUE slots on stack
    - Charly.Stack.grow():       Grow the stack by doubling its size, throw if total limit reached
    - Charly.Stack.shrink():     Shrink to 50% of current size, throw if not possible
  - Quick to switch out, just swap a pointer
  - To improve startup performance of new threads, 8 kilobyte pages can be preallocated

- Refactor interactions with Charly data types which are stored on the heap
  - Write initializer functions for each type
  - How is key enumeration being handled?
    - Some kind of iterator?
  - Types should define their own methods / functionality
  - No external access to private member fields
  - Turn structs into classes to enforce access rights
  - Use a spinlock to sync access to heap types
    - Much better performance than a mutex
    - We only use the lock for a very short amount of time
    - Shared reads / unique locks?

- Architecture changes to worker threads
  - Have a fixed amount of worker threads which are always alive, delivering jobs via some message queue
  - Implement kqueue for async networking

- Basic file io methods

- Look into Fiber architectures
  - Could we already implement this using the Sync.Notifier primitives
  - Create basic runtime scheduler and scheduling methods prototypes
    - See: Go Goroutines, Ruby Fibers / Threads
  - Parallelism using fibers?
  - If implemented, Promises can be removed again
    - They are not needed anymore, since we now have independently running fibers

- 'users.each(->.name)' Syntax
  - Arrow functions which begin with a "." (dot) are parsed as '->$0.foo'

- Smarter stacktraces
  - Timers should contain the stacktrace that led up to its invocation
  - Clearly mark any async boundaries

- File system library
  - C++17 native std::filesystem library

- Todos
  - unit test 'is_a' method for primitive types
  - typeof operator should return the primitive class, not a string
  - helper methods to check variables for specific types
    - throw TypeError exceptions
    - check arrays for specific types
    - check object for specific layout
    - syntax support for type checking?

- Fix the Garbage Collector bug for reals
  - VM::pop_queue is a temporary fix and needs to be revisited in the future

- Bug with try catch statements
  - `break`, `continue` do not drop the active catchtable
  - `break`, `continue`, `return` do not respect the `finally` handler

- Changes to the class system
  - Hide prototype variable
    - Add internal method to add new method to prototype
    - Throw if the method already belongs to a class
    - Configure klass property of function

- `on` handler for try catch statements
  - `
      try {
        ...
      } on IOError {
        ...
      }
    `

- Hoist local variable declarations
  - `
      func foo { bar() } <- would generate a readglobal call
      func bar { ... }
    `

- Implement control access to objects
  - Mark a key as private
    - Can only be accessed via functions which are registered
      member functions of the klass of the object in question
  - Mark a key as constant
    - Key cannot be overwritten or deleted

- Reintroduce multiple inheritance
  - Currently there is no easy way to declare "interfaces", like in other languages
  - Example use case: an EventEmitter interface which adds the needed logic to listen for and emit events
  - How are constructors invoked?
  - How is method lookup performed?
  - Which parent classes have priority?
  - SHould a class and an interface be separate things?

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

- Way to reconcile global variables with undefined symbol warnings at compile-time
  - Special syntax to access global variables?

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

- Integer format methods
  - Wrap the C++ number formatting API

- Ability to freeze objects
  - Simple status bits on every object
    - f1 = freeze_values        When set to 1, no values can be overwritten
    - f2 = freeze_keys          When set to 1, no keys can be added or removed

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
  - GC looses track of VALUES that are stored in the machine stack
  - GC collections need to pause the VM
    - GC collections can be caused inside the VM or inside Worker threads
      - If a collection is started inside a worker thread, the VM needs to be paused


- Fix runtime crash when executing the runtime profiler
  - A frame gets deallocated while the vm is still using it, resulting in the return instruction
    jumping to a nullpointer. This bug has to be related to some place not having the right checks
    in place.
