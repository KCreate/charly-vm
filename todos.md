# Todos

- Diagnostic message emitter abstraction instead of throwing an exception
  - Diagnosts manager gets source code of file and can print pretty
    color highlighted dump of stuff
  - Specific error message for each scenario
  - Puts all error messages into one place
  - General
    - Knows about program source
    - Understands location formation of diagnostic messages
    - Can generate a pretty (with colors on ttys) error format

- Unit tests
  - Check for parser exceptions
  - Operator precedence
  - Spawn statement
    - Needs call syntax to be implemented

- Write some unit tests for ast node location
  - Check row, column and length output

- Refactor node visiting code
  - Remove places where new nodes have to be registered

- Parser
  - parse member, call and index expressions
  - parse expression keywords (yield, spawn, import, await, typeof)
  - parse lists
  - curly braces
    - parse dicts `{a: 1, b: 2}`
    - parse blocks as statements `func foo { foo() { const a = 2; print(a) } }`
  - parse functions
  - parse arrow function
  - parse generators
  - parse classes
  - parse if
  - parse unless
  - parse guard
  - parse switch
  - parse match
  - parse while
  - parse until
  - parse loop
  - parse for
  - parse try
  - some illegal syntaxes can be caught by sometimes parsing newline tokens
    - inside subscript, call parsing methods

- Semantic constraints
  - Desugar syntax constructs
  - Check if keywords are placed at legal positions
  - Only catch errors which would constitute an ill-formed charly program
    not incorrect (buggy) programs. for example `defer return 1` should be an error since
    a return statement is not allowed to be present inside a defer clause
  - Insert missing statements (return null inside empty blocks)
  - Check for illegal identifiers
  - Generate constructor for class
  - Check if super is present inside subclass constructors
  - Class and function literals are rewritten to declarations

- Intermediate representation for charly code
  - Human readable charly bytecode syntax
  - Assembler for charly bytecodes
  - Bytecode to file / line / column mapping
- Value model
- Memory allocator
- Codegenerator
  - Creates instructionblock
  - Allocates constants
- Thread control
- Fiber threads
- Implement basic opcodes
- Memory locking
- REPL support

# Rewrite of the codebase

- open questions
  - JIT compilation of charly opcodes
    - Implement fiber scheduler with makecontext, setcontext methods

  - JVM exception tables
    - Generate exception tables for each function at compile time
      - How is the type id obtained for the exception classes?
      - Can the catchtables be completely removed?
      - Unwinding of block stack for defer and loop constructs

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

  - native async / await
    - builtin promise type
    - async calls a function asyncronously
    - await waits for a promise to finish
      - if promise got rejected, throw error at await

  - different compiler modes
    - repl      code starts executing directly, return yielded value
    - module    code exports file code inside a module function

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
  - generational garbage collection?
    - how are values moved?
    - young, old, permanent generation
    - when writing references to a value, mark the target page as dirty

- solved questions
  - in what thread are native methods executed?
    - each cfunction gets a flag which controls wether it should be executed
      in sync with the fiber or put onto the native worker queue
    - by default, a cfunction gets executed in sync with the fiber
  - REPL support
    - variables in the top level are adressed by name
    - each variable gets registered using the `registerlocal` opcode
    - nested blocks register a new local frame by `setuplocals` and deregister by `unwind`
    - functions store the local block in which they were defined, in case they have to perform name lookups.
    - data structure shouldn't be a simple map, variables should still be accessible by offset
      - this is to allow future optimizations such as inline caching
  - max amount of arguments for a function
    - except in case of functions with argument spreads
    - throw exception on mismatched argument count
  - how are fibers created?
    - allocate a new fiber, call the function, enqueue in coordinator
  - instructionblock bookkeeping
    - constant pool of values allocated during compile-time (similar to python)
  - vm reentrance, how can the VM call charly logic?
    - if we can detect that certain instructions would require us to call back into
      charly code, we could just handle these instructions as charly code
      call a function that handles the opcode and just returns to the old position
      once done
  - remove computed goto, use switch with prediction nodes for branch efficiency
    - inspired by CPython main interpreter loop
    - remove compound opcodes, branch prediction should be sufficient
  - local variable slot allocation with defer statements
    - by currently existing rules, both a and b would end up in the same slot
      ```
      {
        defer {
          let a = 25
        }

        let b = {}
      }
      ```
    - by marking all slots created inside the defer as leaked, they are not overwritten by other slots
  - correct stack unwinding for try/catch and loop statements
    - keep a stack of block states on which new instructions `break`, `continue` operate
    - break and continue statements now correctly unwind exception stack
      - compiler can still optimize as immediate branches if possible
    - defer statements should execute immediately before their scope is exited
    - finally statements should execute synchronously
    - implement via some kind of state machine
    - new instructions
      - SETUPLOOP
        - creates a new loop block, used by break and continue
      - SETUPDEFER
        - creates a new defer block, executes defered blocks at unwind
      - SETUPCATCH
        - creates a catch block, handles thrown exceptions
      - DEFER
        - schedule a block for execution at corresponding unwind
      - UNWIND
        - unwind the top block
      - UNWINDCONTINUE
        - continue unwinding the stack
        - used inside defered blocks
    - finally handlers for catch statements can be implemented using defer statements
  - stacked exceptions
    - exceptions thrown in the handler blocks of try catch statements should
      keep track of the old exception
    - some kind of exception stack per fiber, popped at the end of catch statements
    - pop exception off the exception stack at the end of the catch handler

- usage of exceptions in codebase
  - vm panic handling
  - exception handling
  - performance concerns?

- New features
  - Tuples
  - Value unpacking
    ```
    > const (a, b, c) = foo()

    readlocal foo
    call 0
    unpack 3
    setlocal c
    setlocal b
    setlocal a
    ```
  - Unpacking into functions, arrays
    ```
    > [1, 2, foo, ...foo, 3, 4]

    loadi 1
    loadi 2
    readlocal foo
    puttuple 3
    readlocal foo
    loadi 3
    loadi 4
    puttuple 2
    putlistunpack 3

    > (1, 2, foo, ...foo, 3, 4)

    loadi 1
    loadi 2
    readlocal foo
    puttuple 3
    readlocal foo
    loadi 3
    loadi 4
    puttuple 2
    puttupleunpack 3

    > bar(1, 2, foo, ...foo, 3, 4)

    readlocal bar
    loadi 1
    loadi 2
    readlocal foo
    puttuple 3
    readlocal foo
    loadi 3
    loadi 4
    puttuple 2
    puttupleunpack 3
    callunpack

    ```
  - `for in` statements
  - `defer` statement
  - hidden classes to speed up member lookups
  - inline caches using hidden classes
  - foreign function interface for native dynamic libraries
  - charly module interface
  - block stack
    - loops, try-catch push a block onto the block stack and
  - per-frame data stack, no global stack
  - string interpolation
    - implement using lexer modes
  - garbage collector should keep freelists of different sizes
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
      - range
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
