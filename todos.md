# Todos

# Internal methods system
- Put them into a separate file
- Load methods on demand
  - Maybe use _dlopen_ for this?
  - Consult ruby source code on how to do this
  - Should be rather trivial
- Methods can be obtained in user-space via the get_method method that is injected
  on machine startup.

# Class System
- Add instructions that add methods and properties (static variants also)
  - ClassRegisterProperty
  - ClassRegisterStaticProperty
  - ClassRegisterMethod
  - ClassRegisterStaticMethod
- Inject basic classes at machine startup
  - Keep a reference to these classes somewhere?

# Type data structure
- Remove the klass field as for most types it wont change
- Make sure this doesn't fuck up alignment issues inside the GC
- Types which do need the class field (Object) will store it inside an unnamed field inside
  their own data containers

# Class Construction
- Class is called
- Insert special fields into object
  - 0x00: calling class
- Insert all the classes fields and methods into the object
  - Skip the constructor method
- Check if there is a constructor inside the class
  - Check if there are enough arguments for the constructor
  - Setup an exception handler which deallocates anything if the constructor throws an exception
  - Call the constructor, setting the self value to the newly created object
- If the constructor succeeded, push the newly created object onto the stack

# Garbage Collection
- Mark and Sweep
- GC if there are no free cells available anymore
- The GC needs a pointer to the VM
- The VM has to keep a list of root nodes
  - Stack, Frames, Toplevel
- GC iterates over each root node, recursively marking all nodes
  - If a node has already been visited, return
- Iterate over all heaps and their cells
  - If a cell is not marked, free it, goto next cell
  - If a cell is marked, unmark it, goto next cell
- Return the last newly created free cell to the allocate call

# Memory ownership
- Structs generally own the memory they point to
- This means the compiler can't assume that memory will stick around after it's been created
- Strings
  - The memory created by compiling a string, is owned by the VM, even if the VM doesn't know about the string
  - If the instruction which tell the vm to create a new string is never executed, the string will stick
    around until the end of the machine
    - Solution would be to have a string pool per function, with a refcount to each string
    - Once a function is being deallocated, all strings contained inside that function are removed too
      - TODO: Make sure there are no edge cases for this
  - The compiler rounds up string memory allocation to 128 bytes
- InstructionBlocks
  - IB's are owned by the function they are placed into
  - Once the function is being deallocated, their instructionblock is deallocated as well
- Bound Functions
  - When binding a new self value to a function, the instruction block is copied
  - This is to keep the possibility to optimize the function once the self value is known

# VM deallocation
  - Deallocating the VM should destroy all the memory it can find,
  - Any outside-user is responsible himself to correctly copy everything needed for further program execution
  - VM deallocation order
    - Flush the stack (not via the GC)
    - Flush the top-level
    - Tell the GC to destroy everything it can find (overwrite with 0)
    - Deallocate member objects of the VM
    - Deallocate vm itself

# Operators
- Maybe use templates for this?
  - Copy the operator matrix from the Crystal Charly source. Don't reinvent the wheel.

# Implement all opcodes
- ReadMemberValue
- SetMemberValue
- PutString
- PutArray
- PutClass
- Topn
- Setn
- Throw (only Return is implemented so far)
- Add
- Sub
- Mul
- Div
- Mod
- Pow
- Eq
- Neq
- Lt
- Gt
- Le
- Ge
- Shr
- Shl
- And
- Or
- Xor
- UAdd
- USub
- UNot
- UBNot

# Exception system
- Every type is throwable (the payload is a VALUE type)
- The top-level wraps the entire program in a try catch clause
  - Machine has some logic that handles global uncaught exceptions
    - User-defined?
    - Written in Charly or as an external method?
- Machine has a CatchStack
  - Try-Catch, Switch, While, Until and Loop statements push new entries to the catch table
    - The frame contains a pointer to the currently active catch table
  - The CatchStack is a linked-list of CatchTables
  - CatchTables are allocated by the GC, just like Frames
  - Each CatchTable contains multiple pointers into InstructionBlocks for specific exception types
    - Exception, Break, Continue
    - Return isn't handled by the CatchTable, as it's handled separately via the whole Frame system
    - If a field is a null pointer, travel to the next CatchTable
    - If an exception reaches the top-level
      - Exception: Call uncaught exception method
      - Break and Continue:
        - Create an exception detailing what happened and rethrow it from the original throw location
  - CatchTable contains a pointer to the Frame that was active when the try-catch statements started
  - Struct for a catch table:
    ```cpp
    struct CatchTable {
      ThrowType type;
      uint8_t* exception_handler;
      uint8_t* break_handler;
      uint8_t* continue_handler;
      Frame* frame;
      CatchTable* parent;
    };
    ```
  - Struct for an exception:
    ```cpp
    struct Exception {
      ThrowType type;
      VALUE payload;
    };
    ```

# Define deconstructors for some objects
- Should normal destructors be used?
  - We are already allocating and "constructor" in a funny way, so why use destructors here?
- If we manage all resources in pre-defined types, we won't need a "finalize" function
  - CFunctions might allocate stuff and the deallocation logic might be written inside Charly
  - Maybe provide enough data structures inside the vm, so that deallocation logic inside
    Charly can be mitigated, thus leaving all dealloc logic to the GC.

    This could be achieved by wrapping pointers (maybe?). Thus, C-Bindings would return
    wrapped pointers, optionally defining a function pointer for a destructor function.
    We can keep the correct context object by using the `->*` and `.*` operators.
    See: https://isocpp.org/wiki/faq/pointers-to-members#dotstar-vs-arrowstar
