# Todos

# Remove some vm methods
- VM::read and VM::write are only needed inside opcodes readsymbol and writesymbol
- Allow ReadLocal and WriteLocal to get receive a level
  - Lexical scope makes it possible to calculate all these offsets at compile-time

# Testing
- Unit-test single methods in the VM
- Find a good unit-testing framework
- TravisCI Support?
- Good diagnostics output in CLI
- Automated, no user intervention
- Multiple tests per file?

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

# Objects
- Hashes are just objects with their klass field to the Object class
- Objects contain their klass field in the first slot of their container

# Garbage Collection
- Mark and Sweep
- GC if there are no free cells available anymore
- The GC needs a pointer to the VM
- The VM has to keep a list of root nodes
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

# VM deallocation
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

# Remove useless opcode
- PutFloat doesn't make sense, since this can also be done via PutValue

# Mapping between JITed code and source file locations
- Figure out how JavaScript source maps work and copy the shit out of it
- Mapping between a range of instructions to a range of row/col pairs on compilation?

# Exception system
- The top-level wraps the entire program in a try catch clause
  - The Machine contains some general global uncaught exception handling
    - The user can provide a hook (a function) that will run when an exception was not caught
    - The program terminates once that hook returns
- Machine has a CatchStack
  - Each CatchTable contains a type and a pointer into an instruction block
    - If an exception reaches the top-level
      - Exception: Call uncaught exception method
      - Break and Continue:
        - Create an exception detailing what happened and rethrow it from the original throw location

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
