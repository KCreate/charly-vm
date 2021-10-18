# Todos

- Interpreter value comparison method

- Interpreter value string output
  - How are heap types formatted to strings?
  - How are custom objects formatted?

- Implement REPL in charly
  - Use the standard library to read, compile and execute input statements
  - Implement a clean interface to execute code in the runtime from a global context
    - Execute and return immediately
    - Execute and wait for return value
  - Would simplify result handling and GC liveness logic if all this functionality was implemented
    directly in Charly

- Implement lowest execution tier as a template-JIT instead of bytecode interpreter?
  - Would allow me to get some easy experience with building and integrating the JIT into the system
    instead of having to tack it on later
  - Generation could be a simple templating system, outputting assembly templates for each bytecode
  
- Implement os.h class which implements platform-specific interactions with the OS
  - Platform specific custom implementations of make_fcontext and jump_fcontext

- Concurrent Garbage Collector
  - Load / Write Barriers
    - When does the runtime have to check the forward pointers of a cell?
    - How does this mechanism work?
    - How is it implemented?
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
  - Classes cannot be modified at runtime. They can only be subclassed
  - Objects creates from classes cannot have new keys assigned to them
    - They are static shapes that can only store the properties declared in their class
  - For a dynamic collection of values use the dict primitive datatype
    - Small dictionaries could be laid out inline and take advantage of inline caches

- Polymorphic inline caches
  - Caches should be able to hold at least 2-3 entries
  - Cache eviction?
    - Age, LRU
  - Avoid constant inline cache updates
    - Should learn from use and only cache top 2-3 frequent entries
    - Reset this heuristic after some time to adapt to new workloads

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
  - vm reentrance, how can the VM call charly logic?
    - if we can detect that certain instructions would require us to call back into
      charly code, we could just handle these instructions as charly code
      call a function that handles the opcode and just returns to the old position
      once done

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
