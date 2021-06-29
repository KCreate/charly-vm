# Todos

- Remove 'dump_asm' flag (not useful since all relevant info is displayed already with 'dump_ir')

- CLI flag for maximum worker count

- Write some unit-tests
  - Write some unit-tests for the generated module
  - Symbol table
  - String tables of functions
  - Correct meta information

- Remove stackcheck instruction
  - Handle implicitly at begin of function calls

- Rewrite pseudo-instructions to their real equivalent

- Document the calling convention / frame locals layout somewhere

- Implement some basic opcodes
  - during this part of development I want to figure out how to implement the write / read barriers
  - figure out how compiled programs are loaded into the machine
  - figure out main bytecode execution loop
    - interaction with scheduler

- Concurrent Garbage Collector
  - Phases
    - Idle                    | Application is running normally, GC is not running
    - Idle -> Marking         | Mark roots and start background marking
    - Marking                 | Heap tracing, write barriers append to queue drained at next pause
    - Marking -> Evacuation   | Drain SATB buffers and initialize evacuation phase
    - Evacuation              | Heap objects get relocated, write barriers access forward pointer
    - Evacuation -> UpdateRef | Initialize update ref phase
    - UpdateRef               | Heap references to old objects get updated, write barriers update references
    - UpdateRef -> Idle       | Finalize garbage collection, prepare for idle phase
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
        - During the Init-Evacuate pause, allocate each alive cell in From-regions a slot
          in the corresponding To-regions
        - Evacuation can atomically check if the final copy has already been created / initialized
      - Each from-space knows in what to-space it will end up
        - Need some easy way of knowing which offset each cell ultimately ends up at
    - How are the copies that are created during a contended evacuation recycled?
      - Mechanism to un-allocate a cell, works only if no other cell was allocated since
        - Since another copy will be allocated in another region, we can just unwind the last
          allocation and reuse it for the next one
  - UpdateRef
    - Init phase
      - Update root set
    - Concurrent phase
      - Update references to objects in from-spaces
    - Application threads
      - Update references

- Race condition inside allocator
  - Free regions might already be gone by the time thread reaches the freelist check
    after calling expand_heap

- Fiber stack growth mechanism via mmap?
  - Reserve address space for 8 megabytes per fiber
  - Map in actual memory pages as they are needed
  - Unmap pages when they are no longer needed

- Implement native mode mechanism
  - Worker threads that are inside native mode, that exceed some timeout (20ms?) will be
    marked as preempted by the system monitor thread.
    - The runtime will release the processor from the worker thread and will return it to the idle
      list of processors
    - The system monitor now detects that there are idle processors, but no idle worker threads,
      so the runtime will start a new worker thread that acquired the processor
    - Once the prempted worker thread returns from the native section it will check if it was
      preempted
      - If the worker was pre-empted, we try to reacquire the old processor
      - If reacquiring the old processor fails, try to acquire another idle processor
      - If there are no idle processors, idle the current worker thread and reschedule the running
        fiber into the global runqueue

- Reimplement make_fcontext and jump_fcontext in hand-written assembly
  - Try to understand what boost does in its own implementation

- Small locks
  - Implement from experiment code in old charly repo
    - See: https://github.com/KCreate/charly-vm/blob/main/experiments/small-locks.cpp
  - Types of locks
    - FairLock
      - Access to lock is given in strict FIFO order.
    - BargingLock
      - Threads can acquire the lock multiple times in rapid succession, even if another thread
        requested the lock in the meantime
    - SharedLock
      - See: https://eli.thegreenplace.net/2019/implementing-reader-writer-locks/

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

- Value model
  - Each heap cell should be divided into 8 byte cells
    - Allows for more detailed inline caches as builtin properties can be
      cached in the same way that object properties may be cached
  - Classes cannot be modified at runtime. They can only be subclassed
  - Objects creates from classes cannot have new keys assigned to them
    - They are static shapes that can only store the properties declared in their class
    - For a dynamic collection of values use the dict primitive datatype
      - Small dictionaries could be laid out inline and take advantage of inline caches

- Implement basic opcodes

- Memory locking

- REPL support

- Pointer Tagging support double float
  - Use some bits of the mantissa for the pointer tag
  - Reduces accuracy but should be okay

- Polymorphic inline caches
  - Caches should be able to hold at least 2-3 entries
  - Cache eviction?
    - Age, LRU
  - Avoid constant inline cache updates
    - Should learn from use and only cache top 2-3 frequent entries
    - Reset this heuristic after some time to adapt to new workloads

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

- Different sized regions?
  - Regions with small cells, regions with large cells
  - Maybe have the ability to decide at allocation time wether a region should be split
    into small-object cells or large-object cells.
  - Each worker would contain two active regions at any point. One for small objects one for big ones

- Fiber scheduler with fcontext_t
  - How to protect the stack from overflow?
    - Protect memory page immediately after the stack to trap on accesses

- Implement mechanism to wait for GC cycle to complete when heap could not be expanded
  - Instead of failing the allocation, threads should attempt to wait for a GC cycle before they fail

# Rewrite of the codebase

- open questions
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
