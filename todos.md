# Todos

- Leaked variables need to be allocated on heap frames, not stack frames
  - Special instructions to access heap variables?
  - Needs changes to local variable allocator

- Fix syntax for object unpack assignments
  - Parser treats it like a source block
  - Introduce new syntax for a single block
    - Automatically detect wether its a block or dict literal???

- Think about inline caches
  - Different types of inline caches
    - PropertyOffsetCache   maps class id to property offset
    - GlobalVariableCache   stores global variable offset
    - MethodCache           maps class id to functions
    - BinaryOpCache         cache types of binary operands
    - UnaryOpCache          cache types of unary operand
  - IRBuilder will have to allocate inline caches per function
    - Separate table for inline caches or directly encoded into bytecode?
  - Polymorphic inline caches
    - Caches should be able to hold at least 2-3 entries
    - Cache eviction?
      - Age, LRU
    - Avoid constant inline cache updates
      - Should learn from use and only cache top 2-3 frequent entries
      - Reset this heuristic after some time to adapt to new workloads

- Runtime representation of functions
  - How does the `makefunc` instruction create a new function?
  - Shared function info?
  - Bytecode
  - Keep possibility of future JIT compiler in mind

- Should classes be static?
  - Classes would, like functions, get their data directly embedded into the bytecode
  - New instances of the class all share common data encoded in the bytecode

- Concurrency
  - Boost fcontext for cheap context switches
  - Signal handling?
  - Core concept of concurrency in charly should be the Channel
  - Channel
    - Concurrent queue
    - Buffered or unbuffered
    - Main method of data sharing and fiber synchronisation
  - Spawn statements return an unbuffered channel
    - Yields from the created fiber are pushed into the channel
  - Promises and other more finegrained abstractions can be built on top of Channels
  - User has access to underlying fiber primitives but should be an implementation detail
  - Await keyword is used to read from a channel
  - Class specific await behaviour can be achieved by overriding some handler method
  - Global symbol table with thread local intermediate caches

- Concurrent Garbage Collector
  - Different properties of garbage collector types
    - Pause times
    - Application performance overhead
    - Heap fragmentation (Cache performance)
    - Compaction? Can the heap shrink over time
  - Shenandoah collector from OpenJDK
    - Mark word containing forward pointer
    - Read / Write barriers for object accesses

- Value model
  - Each heap cell should be divided into 8 byte cells
    - Allows for more detailed inline caches as builtin properties can be
      cached in the same way that object properties may be cached
  - Classes cannot be modified at runtime. They can only be subclassed
  - Object creates from classes cannot have new keys assigned to them
    - They are static shapes that can only store the properties declared in their class
    - For a dynamic collection of values use the dict primitive datatype
      - Small dictionaries could be laid out inline and take advantage of inline caches

- Task scheduling
  - Cannot have a single global task queue, too much overhead
  - Every worker thread needs to get their own task queue
    - Implement work stealing between the threads

- Memory allocator
- Codegenerator
  - Creates instructionblock
  - Allocates constants
- Fiber threads
  - Write custom scheduler using setjmp/longjmp
  - Allocate custom stack (4kb) per fiber
    - How to grow / shrink stack automatically?
    - How does Go do this?
  - Using setjmp/longjmp is orders of magnitude faster than ucontext or a regular thread context
    switch because it does not involve any system calls.
  - ucontext is useless in this case because it involves system calls
  - Allocating a dummy page with mprotect(PROT_NONE) after the stack (before because it grows downwards)
    allows us to detect stack-overflows
    - I don't know if we could recover a fiber with an overflowed stack
  - Ideally the stack overflow would be detected before it actually happens so we can
    gracefully throw a Charly or grow the stack to accomodate more data
- Implement basic opcodes
- Memory locking
- REPL support

- Exception tables
  - Exception tables should be split into non overlapping segments

- Function frames can be allocated on the heap or on the stack
  - Each function has a bool property on wether the frame locals
    should be allocated on the stack or heap

- Argument-Indexing identifiers
  - $0, $1 syntax
  - In-bounds identifiers can be replaced by the actual arguments

- Unit tests
  - Charly source file containing annotations for expected output in stdout
  - Similar to Swift unit tests
    - Example: https://github.com/apple/swift/blob/main/test/Concurrency/async_conformance.swift

- Computed property methods
  - clases should be able to define property methods like `property length { ... }`
  - inline caches could store the function reference and call it directly

- Stream coloring on macOS
  - termcolor::_internal::is_colorized returns false, manually executing the function body returns true???

# Rewrite of the codebase

- open questions
  - JIT compilation of charly opcodes
    - Implement fiber scheduler with makecontext, setcontext methods

  - JVM exception tables
    - Generate exception tables for each function at compile time
      - How is the type id obtained for the exception classes?
      - Can the catchtables be completely removed?

  - object handles that also work with immediate encoded values (Handle<Object>, Handle<VALUE>)
    - std::is_base_of can be used to enforce subclasses of Header only for Handle type
    - thread safe object handles
      - different threads might have handles to the same value
      - refcount (immortal if rc > 0) needs to be handled in an atomic way

  - exception handling
    - collecting the stack trace should not be done within charly
      - it should be possible, but exceptions should also be able to be created fully inside
        c++ code, without calling into charly code
    - well known class serving as base class of exception class

  - uncaught exceptions
    - machine control exceptions
      - SystemExit
      - FiberExit
      - GeneratorExit
    - regular exceptions

  - how are iterators implemented
    - wrappers around builtin collection types (list, dict, tuple)
    - when iterating over non indexable types (like dict) an iterator to the underlying data structure
      is stored and incremented for each read operation
      - iterator invalidation?
    - if passed a value which can't be trivially turned into a iterator call the to_iterator function on it
    - base overload throws an error (could not turn into iterator)
    - generators return tuple (value, done)
      - iterators also return tuple (value, done)
    - should generators be closeable?
      - GeneratorExit exception, child class of BaseException, cannot be caught by catchall Exception
      - Cleans up the generators defer handles

  - Fiber operations
    - Create          Create new fiber, with function and arguments, not running by default
    - Suspend         Suspend current fiber
    - Schedule        Schedule at timestamp
    - Throw           Resume suspended fiber, throw exception
    - Resume          Resume other fiber
    - Call            Resume other fiber, suspend current fiber
    - Yield           Suspend current fiber, resume callee fiber
    - Close           Resume fiber, throw FiberExit
    - Wait            Resume current fiber when other fiber exits

  - inline caching of object attributes and methods
    - cache property and global name lookups
      - property caches can never be invalidated, if the map-id matches, the offset is correct
      - global name lookups can be invalidated
    - classes are constant once they are defined, no methods can be added
    - how are inline caches synchronized?
      - inline smalllock
      - the lock only needs to be held when writing to the cache
      - if some other thread writes to the cache and creates a new map before some other thread has
        finished its read operation, it does not matter since the old indices would not be invalidated
      - what if an inline cache is highly contended?
        - if inline cache gets updated more often than some defined limit, disable the cache site
    - layout of cache sites
      ```
      struct InlineCache {
        struct Entry {
          uint32_t mapid;
          uint32_t offset;
        };

        std::atomic<Entry> entry;
        bool enabled;
        tinylock lock;
      };
      ```

- solved questions
  - in what thread are native methods executed?
    - each cfunction gets a flag which controls wether it should be executed
      in sync with the fiber or put onto the native worker queue
    - by default, a cfunction gets executed in sync with the fiber
  - max amount of arguments for a function
    - except in case of functions with argument spreads
    - throw exception on mismatched argument count
  - how are fibers created?
    - allocate a new fiber, call the function, enqueue in coordinator
  - vm reentrance, how can the VM call charly logic?
    - if we can detect that certain instructions would require us to call back into
      charly code, we could just handle these instructions as charly code
      call a function that handles the opcode and just returns to the old position
      once done
  - remove computed goto, use switch with prediction nodes for branch efficiency
    - inspired by CPython main interpreter loop
    - remove compound opcodes, branch prediction should be sufficient
  - stacked exceptions
    - exceptions thrown in the handler blocks of try catch statements should
      keep track of the old exception
    - some kind of exception stack per fiber, popped at the end of catch statements
    - pop exception off the exception stack at the end of the catch handler

- hidden classes to speed up member lookups
- inline caches using hidden classes
- foreign function interface for native dynamic libraries
- signal handling
  - background thread which waits for signals to arrive, then schedules a signal handler
    function via the coordinator, passing signal information to it
  - charly code can register for specific signals and pass a callback handler
- list of builtin types
  - debug / meta
    - dead          placeholder for free gc cells
  - literal
    - int
    - float
    - boolean
    - null
  - container
    - list
    - dict
    - tuple
  - language
    - string
    - class
    - object
    - function
    - generator
    - iterator
    - promises
  - vm-internals
    - frame
    - block
      - locals
      - catchblock
      - loopblock
      - deferblock
    - fiber
    - file
    - socket
    - symbol
    - objectmap
    - codeobject
  - c-interop
    - clibrary
    - cfunction
    - cpointer
