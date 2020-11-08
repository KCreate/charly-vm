# Todos

- VM Refactor
  - Implementation timeline
    - Remove configurability of stdout and stderr streams
      - Always print debug messages to stderr, not stdout
    - C methods refactor
      - C methods should not have access to the VM internals
      - Only receive the self value and arguments passed to the function
      - How do C methods throw exceptions?
    - Make the Garbage collector a static singleton, securing memory allocation with a
      mutex for the beginning.
    - Property access methods can actually be removed again, make properties atomic.
      - This won't work for all properties, so some accessor methods will still be needed.
      - I don't plan on putting the locking code into the accessor methods but will probably go
        for some RAII style locking thing which locks a value during the scope of some block.
        I'll want a UniqueValueLock and a SharedValueLock (typedef ValueLock).
    - Fixed amount of native worker threads, waiting for jobs via some job queue
    - Some global values can be stored as atomics, no mutex needed
    - Remove `halt_after_return`, replace with dedicated instructions
    - Refactor thread system to use the states and ability to wait for certain signals
      - GC goes into its own thread
      - Fibers create a Fiber heap object that mirrors the real fiber

  - What threads are active in our runtime (N = logical CPU cores)
    - 1x Coordinator thread (main thread)   | Timers, tickers, task queue, fiber queue
    - 1x Async networking thread            | Epoll, kqueue
    - Nx Fiber executor threads             | Executes fibers
    - Nx Native code worker threads         | Executes async C code

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
        - What are protected sections for? Interactions with important queues can be abstracted away via some
          context object passed to the native function, disallowing direct access to important data structures.
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
      - Deadlock could happen if a code section currently holds a lock of some heap value
        allocates something inside the critical section and then triggers a GC. For the GC to be able
        to start, all other threads need to be at well-known places. If another thread is waiting for the
        value lock to be released, it will wait forever and the runtime will freeze.
        - This could be resolved by somehow figuring out wether a thread is currently waiting for a
          lock to be released. This would obviously only work with our well-known heap value locks.
          Maybe some wrapper class which updates the thread status once it starts waiting?

- Move comparison operations out of the VM into external methods or directly into the value classes

- Changes to the class system
  - Hide prototype variable
    - Add internal method to add new method to prototype
    - Throw if the method already belongs to a class
    - Configure klass property of function

- Smaller bytecode instructions
  - Some instructions use a 32bit integer to store a count field, which is way too much.
  - Lots of these can be replaced with 8 bit integers
  - Also switch to using a flags byte instead of using multiple bytes for boolean flags in instructions

- Fibers can be represented by GC allocated structures, making them accessible to charly code
  - We simply store a `set<Fiber*>` of all active fibers
  - Operations can be performed directly on the fiber value itself, without having to worry
    about syncing it with charly space.

- Lots of data structures used by the VM can actually be represented using these GC structures
  - This makes it a lot easier to interact with them from charly code.
  - Timers and tickers can also be represented using this scheme
  - Task queue can also be represented by GC structures

- Make instructionblocks values allocated by the GC, thus accessible to charly code
  - Also allows instructionblocks to be deallocated once they are no longer needed
  - Would allow us to store location information in a format easily accessible via the VM

- Write a new thread-safe queue class that is easily traversable for the GC
  - For the beginning this could just be a simple linked list with a mutex

- Implement a Heap class
  - Needed to efficiently store the current active timers.
  - Getting the next timer to fire becomes very easy.
  - Tickers can also be stored inside a heap

- New instructions to replace functionality in native C methods
  - Timer and ticker handlers
    - timercreate
    - tickercreate
    - timerclear
    - tickerclear
  - Fiber instructions
    - pushfiberid
    - fibercreate
    - fibercall
    - fibersuspend
    - fiberresume
    - fiberyield
  - Native task scheduling
    - schedulenativetask
  - Type cast instructions
    - castint
    - castfloat
    - castnumber
    - caststring
    - castboolean
    - castsymbol
  - Function dynamic call
    - calldynamic
    - callmemberdynamic
  - Function bound self
    - setboundself
    - clearboundself
  - Frame handling
    - putlocalframe
  - Delete instructions
    - deletemembersymbol
    - deletemembervalue
    - deleteglobal
  - Type assertions
    - checktypeclass
    - checktypeobject
    - checktypearray
    - checktypestring
    - checktypefunction
    - checktypecfunction
    - checktypeframe
    - checktypecatchtable
    - checktypecpointer
    - checktypenumber
    - checktypeboolean
    - checktypenull
    - checktypesymbol

- Reimplement default argument in a smarter way
  - Instead of checking the arguments length, generate the necessary code to generate
    the default argument in sequence, storing offsets to be applied to the instruction pointer
    when calling the function with different amounts of arguments
  -
    ```
    // Initial code
    func foo(a = 1, b = 2) = a + b

    // Becomes:
    func foo(a, b) { // required argc = 0
      .A0:      <- entry point for argc = 0
        a = 1
      .A1       <- entry point for argc = 1
        b = 2
      .A2       <- entry point for argc >= 2
        return a + b
    }
    ```

- Automated CI testing
  - TravisCI supports C++ builds and also has nice GitHub integration
  - https://docs.travis-ci.com/user/languages/cpp/

- Change enums to enum classes
  - https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/

- Implement an assembler for charly bytecodes
  - Should be accessible via C++ to generate an InstructionBuffer from the assembly representation
  - Should be accessible via Charly code via some __asm__("") keyword
  - Might allow us to bootstrap the VM from charly bytecodes, making the compiler only accessible via
    external C methods
  - Move VM logic out of external C methods and into actual opcodes, now accessible via
    asm calls
  - Labels should be supported

- Implement a custom hashmap for containers to allow concurrent reads and writes to separate buckets
  - Each bucket has its own lock, which allows multiple writers concurrent access to different buckets
  - To grow the queue we acquire all the bucket locks, increase the size, copy the old elements over
    - Waiting threads need to wait via some condition_variable

- Enforce max string length of uint32_t
- Enforce max array length of uint32_t
- Enforce max frame lvarcount of uint16_t

- Communication between charly and C code
  - C code can send data back to the charly world by appending an entry to the task
    queue, invoking some call with some data.
  - There is no way that charly code can communicate with running C code.
    - Could be implemented via some kind of message queue accessed via a CPointer?
  - Long running C code runs either inside a Fiber thread or inside a C worker thread.
    - Both these threads are constrained and if all of them are executing long running C code,
      this will starve the entire machine. There needs to be some mechanism to split off a thread
      and keep executing it in the background.

- Remove panic system from the VM, turn into asserts

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
  - How is key enumeration being handled?
    - Some kind of iterator?
  - Use a spinlock to sync access to heap types
    - Much better performance than a mutex
    - We only use the lock for a very short amount of time
    - Shared reads / unique locks?

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

- locale handling?
  - localization framework
  - time methods need localisation

- calendar time operations
  - add a day, add a year, add 20 years
    - should automatically calculate leap days and stuff like that
