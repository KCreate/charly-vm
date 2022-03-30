# Todos

- Nice, clean way to declare functions of builtin classes
    - In the old charly version I was able to declare builtin classes directly inside charly code
    - The same should be at least somewhat possible here too

- RawInstance::field_at index should be uint8_t as maximum field count is 256

- Load length of string, bytes and tuple types

- Shape ancestor table for more efficient subclass checks

- Make declaring and writing to a new global variable an atomic operation
    - Currently another thread could assign to the global variable before the declaring thread could write to it

- assert statement
    - Similar to java builtin assert statement
    - Throws an AssertionError exception containing the original source of the assert statement

- Functions should be able to be marked as final
    - Subclasses cannot override these functions

- Huge objects
    - Some very big tuples do not fit into a single heap object
    - Space for the object must be allocated on the global heap
    - Actual instance only stores a pointer and a size

- Detect variables declared via 'let' that are never written to
    - Detect via VariableAnalyzerPass
    - Can be automatically changed to const variable
    - Allows further optimizations to be made later on

- Functions should copy referenced constants into themselves when they are created

- Cache shapes in processors

- Getter and setter functions
    - Can be a flag on functions

- instanceof operator
    - Used to check if a value is or extends a class

- Charly test suite
    - Implement unit testing framework
    - Build test suite for charly code
        - Structure and test cases can be largely copied from the old repo

- Generators
    - Remove Generators from the language
    - yield is now implemented for the block callback syntax

- Modules and abstract methods
    - Modules can declare functions
    - Modules cannot declare properties
    - Module functions can be marked abstract
    - Module functions are copied into the classes function table
    - Classes can implement modules by `class A extends B implements C {}`
        - Unimplemented abstract methods cause errors

- Rethink iterator system
    - Iterators shouldn't be a separate data structure but should be implemented in charly code
    - Iterators are created by calling the `iterator(old = null)` method
        - When passed `null` as argument, a new iterator is created
        - When passed an iterator as argument, the iterator gets incremented to the next position
        - When the `iterator` method returns null, the iterator has finished
    - Iterator values are fetched by calling `iterator_value(iter)`

- Operator overloading
    - Every operator should be overloadable
    - Certain keywords should also be overloadable
        - `await`
    - Some language constructs should be overloadable
        - Index access, Index assignment

- Alternate way of declaring constructor functions
    - Constructor has to be called via `<ClassName>.<ConstructorName>`
        - Internally a static function marked as a class constructor
        - Class constructors get special code generation
            - Create new object instance of correct size and return that
            - User code cannot return custom value from constructor
            - If class has a parent class, super must be called
                - How can different parent class constructors be called?
                - `super.new()` ??
    - Replace `func` keyword with `construct` for constructor functions
    - If neither `func` or `construct` are given, assume func
      ```
      class Container {
          construct new(size = 32, initial = null)
          construct copy(other)
          construct empty()
      }
      ```

- Charly runtime executes as long as there are active (or potentially active) threads
    - Potentially active threads are threads that are waiting for some resource to finish
        - E.g timers or network requests

- Small locks
    - Grow concurrent hash table of thread queues
    - Implement spinning

- Ability to await multiple fibers and handle the first that returns
    - Similar to what the `select` statement in Go does

- Concurrency
    - Signal handling?
    - Channel
        - Buffered or unbuffered
            - Channels can be created with a size
            - Unbuffered channels have size 0 and function as a rendevouz point for two fibers
            - Buffered channels have size 1+
                - Reads block when the channel is empty
                - Writes block when the channel is full
        - `<-` channel operator?
            - Can be used to read from a channel `const value = <- channel`
            - Can be used to insert into a channel `channel <- value`

- Number to int / float convenience functions

- Post-Statement if, while

- Shorthand callback syntax
    - Syntax
        - `5.times -> {}`
        - `list.map -> $ * 2`
        - `list.each -> (e, i) { do_stuff(e, i) }`
        - `10.upto(100) -> (i) {}`
        - `func each(&) { yield 1; yield 2; yield 3 }`
        - `func each(&block) { block(1); block(2); block(3) }`
    - Functions can call the passed block via the yield statement
        - Implement ability to yield multiple, comma-separated values
    - Calling a function that accepts a block without a block, returns an iterator for that function
    - Update yield unit tests

- Fixed size 4 byte instruction encoding
    - Quickening
        - General opcodes must determine the types of input operands
        - Replace original opcode with more detailed variant
        - Thread updates opcodes atomically
        - How are the cache sites synced?
        - Quickened opcodes require some memory to store their cached data
            - Can the compiler determine the total amount of cache sites needed per function?
            - Can the runtime easily determine which address maps to which cache site?
                - Store opcode address in cache slot and perform a linear scan on the first opcode swap
    - Inline caches
        - Cache types
            - Simple Value - stores a single RawValue
            - Property index - store a shape id and a property index
            - Poly Property index - store up to 4 shape ids and property indices
        - Transition between opcodes could lead to insonsistent views of caches
            - Quickened opcode contains index into cache table of function
            - Make index 0 invalid
                - A thread that wishes to update an opcode does an atomic cas, setting the initial cache index to 0
                - Other threads fall back to slow path if they encounter a 0 cache index
                - Only the thread that successfully changed the opcode gets to allocate a cache slot
                - Opcodes in these transition stages may not be changed by other threads

- Refactor symbol table
    - There is a global symbol table that is the ultimate source of truth
    - Newly created strings are added to the global symbol table
    - Each processor has a local cache of symbols that it updates whenever a new symbol is requested

- Optimize global variables
    - Implement the id system, ditch the global hashmap, it is just a temporary hack

- Move most VM logic into the schedulers threads
    - Brainstorm how these threads control each other
    - Execute garbage collector inside a scheduler thread
    - System monitor thread?
        - Detect threads in native mode that exceeded their timeslice
        - Trigger GC when memory gets low

- Setup clang-tidy configuration and fix warnings and errors
    - Also attempt to reduce the amount of disabled compiler flags in the CMakeLists files

- Optimize frame size
    - Frames are about 800 bytes minimum even on release builds
    - about 500 bytes come from the Interpreter::execute function
    - about 150 bytes come from the Interpreter::call_function

- Implement lowest execution tier as a template-JIT instead of bytecode interpreter?
    - Would allow me to get some easy experience with building and integrating the JIT into the system instead of having
      to tack it on later
    - Generation could be a simple templating system, outputting assembly templates for each bytecode

- Implement os.h class which implements platform-specific interactions with the OS
    - Platform specific custom implementations of make_fcontext and jump_fcontext

- Concurrent Garbage Collector
    - Load / Write Barriers
        - When does the runtime have to check the forward pointers of a cell?
        - How does this mechanism work?
        - How is it implemented?
    - Phases
        - Idle | Application is running normally, GC is not running
        - Idle -> Marking | Mark roots and start background marking
        - Marking | Heap tracing, write barriers append to queue drained at next pause
        - Marking -> Evacuation | Drain SATB buffers and initialize evacuation phase
        - Evacuation | Heap objects get relocated, write barriers access forward pointer
        - Evacuation -> UpdateRef | Initialize update ref phase
        - UpdateRef | Heap references to old objects get updated, write barriers update references
        - UpdateRef -> Idle | Finalize garbage collection, prepare for idle phase
    - Concurrent Marking
        - Init phase
            - Workers mark root set
            - New allocations marked grey
            - Mark all VM reachable objects
            - Arm SATB buffers
        - Concurrent phase
            - Traverse application and mark reachable objects
            - Collect statistics and information about heap regions
        - Application threads
            - Process deletes
            - Write barriers
            - Read barriers
    - Evacuation
        - Init phase
            - Disable and mark values from SATB buffers
            - Finish marking
            - Build From-To evacuation mappings
            - Evacuate worker root set
        - Concurrent phase
            - Traverse From Regions and evacuate cells to To-regions
        - Application threads
            - Evacuate in barriers
        - How do evacuated cells know where they end up?
            - Build mapping of From-spaces to To-spaces
                - During the Init-Evacuate pause, allocate each alive cell in From-regions a slot in the corresponding
                  To-regions
                - Evacuation can atomically check if the final copy has already been created / initialized
            - Each from-space knows in what to-space it will end up
                - Need some easy way of knowing which offset each cell ultimately ends up at
        - How are the copies that are created during a contended evacuation recycled?
            - Mechanism to un-allocate a cell, works only if no other cell was allocated since
                - Since another copy will be allocated in another region, we can just unwind the last allocation and
                  reuse it for the next one
    - UpdateRef
        - Init phase
            - Update root set
        - Concurrent phase
            - Update references to objects in from-spaces
        - Application threads
            - Update references

- Implement native mode mechanism
    - Worker threads inside native mode that exceed their alloted timeslice will be marked as preempted by the system
      monitor thread.
        - The runtime will release the processor from the worker thread and will return it to the idle list of
          processors
        - The system monitor now detects that there are idle processors, but no idle worker threads, so the runtime will
          start a new worker thread that acquires the processor
        - Once the prempted worker thread returns from the native section it will check if it was preempted
            - If the worker was pre-empted, we try to reacquire the old processor
            - If reacquiring the old processor fails, try to acquire another idle processor
            - If there are no idle processors, idle the current worker thread and reschedule the running fiber into the
              global runqueue

- Polymorphic inline caches
    - Caches should be able to hold at least 2-3 entries
    - Cache eviction?
        - Age, LRU
    - Avoid constant inline cache updates
        - Should learn from use and only cache top 2-3 frequent entries
        - Reset this heuristic after some time to adapt to new workloads

- Argument-Indexing identifiers
    - '$' refers to first argument
    - '$$' refers to second argument
    - '$$$' refers to third argument, and so on...
    - '$<n>' refers to the nth argument
        - '$1', '$2', '$10' and so on...
    - In-bounds identifiers can be replaced by the actual arguments

- Efficient string concatenation
    - Thread can allocate a temporary RawLargeString as an intermediate buffer
        - Copy and rollback if size exceeds RawLargeString max size
        - Set size after concatenation has finished
        - TAB can get a safety flag to disable allocations during this time
    - Avoiding intermediate copies when calling to_string methods inside FormatStrings
        - to_string could receive some kind of stream object, which appends data to some background buffer

- Unit tests
    - Charly source file containing annotations for expected output in stdout
    - Similar to Swift unit tests
        - Example: https://github.com/apple/swift/blob/main/test/Concurrency/async_conformance.swift

- Computed property methods
    - clases should be able to define property methods like `property x { get { return get_x() } set(v) { set_x(v) } }`
    - inline caches could store the function reference and call it directly

- Implement mechanism to wait for GC cycle to complete when heap could not be expanded
    - Instead of failing the allocation, threads should attempt to wait for a GC cycle before they fail

# Rewrite of the codebase

- open questions
    - uncaught exceptions
        - machine control exceptions
            - SystemExit
            - FiberExit
            - GeneratorExit
        - regular exceptions

    - how are iterators implemented
        - wrappers around builtin collection types (list, dict, tuple)
        - when iterating over non indexable types (like dict) an iterator to the underlying data structure is stored and
          incremented for each read operation
            - iterator invalidation?
        - if passed a value which can't be trivially turned into a iterator call the to_iterator function on it
        - base overload throws an error (could not turn into iterator)
        - generators return tuple (value, done)
            - iterators also return tuple (value, done)
        - should generators be closeable?
            - GeneratorExit exception, child class of BaseException, cannot be caught by catchall Exception
            - Cleans up the generators defer handles

    - Fiber operations
        - Create Create new fiber, with function and arguments, not running by default
        - Suspend Suspend current fiber
        - Schedule Schedule at timestamp
        - Throw Resume suspended fiber, throw exception
        - Resume Resume other fiber
        - Call Resume other fiber, suspend current fiber
        - Yield Suspend current fiber, resume callee fiber
        - Close Resume fiber, throw FiberExit
        - Wait Resume current fiber when other fiber exits

    - inline caching of object attributes and methods
        - cache property and global name lookups
            - property caches can never be invalidated, if the map-id matches, the offset is correct
            - global name lookups can be invalidated
        - classes are constant once they are defined, no methods can be added
        - how are inline caches synchronized?
            - inline smalllock
            - the lock only needs to be held when writing to the cache
            - if some other thread writes to the cache and creates a new map before some other thread has finished its
              read operation, it does not matter since the old indices would not be invalidated
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
    - vm reentrance, how can the VM call charly logic?
        - if we can detect that certain instructions would require us to call back into charly code, we could just
          handle these instructions as charly code call a function that handles the opcode and just returns to the old
          position once done

- hidden classes to speed up member lookups

- inline caches using hidden classes

- foreign function interface for native dynamic libraries

- signal handling
    - background thread which waits for signals to arrive, then schedules a signal handler function via the coordinator,
      passing signal information to it
    - charly code can register for specific signals and pass a callback handler

- list of builtin types
    - debug / meta
        - dead placeholder for free gc cells
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
