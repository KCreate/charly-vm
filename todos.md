# Todos

# Refactoring and code structure
- Remove nested namespaces, have only one namespace called Charly
- Users should be able to use only parts of the VM (without the instruction decoder for example)
- Memory Allocation
  - The Garbage Collector can give you memory in two different ways
    - Direct Reference to Cell
    - Boxed reference, freed when box is deallocated
      - Box can be invalidated and the cell can be unboxed
- Classes
  - Constants
    - Contains all constants the vm exports
    - Format: k + _ClassName_ + _ConstantName_
      - e.g:
        - kStringMaxSize
        - kMemoryInitialStringHeapCount
        - kMemoryInitialObjectHeapCount
  - Machine
    - Memory
      - SymbolTable (mapping between integers and strings)
      - CellPool (managed memory cells of variable size)
      - StringCell, ObjectCell, ArrayCell, etc...
    - CPU
      - Object, Array etc.
      - ControlFrame
      - CatchTable
    - CompilationContext
      - Location
      - Lexer
        - Token
      - Parser
        - ASTNode (unchanged, sugared syntax tree)
      - SemanticTransformer
        - SemanticNode (desugared syntax tree with semantic information and compile-time transformations)
        - Also does optimizations
      - IRGenerator (transforms SemanticNodes into IRNodes)
        - FunctionBuffer
          - InstructionBuffer
          - ChildBuffers (list of child function buffers)

# Calling and object storage convention
- Come up with a calling convention
  - Define where arguments end up inside an environment
  - *arguments* field?
    - Copy of the arguments which are inserted into the stack
    - Updating an argument in the environment won't update the value inside the *arguments* array.
  - self value is being passed via the frames
    - You can't modify the self value, only read from it
  - Passing 3 arguments to a method which only takes 2 arguments, shouldn't put the last argument
    into the container. Only *argc* arguments should be inside the environment.
    - The rest of the arguments should still be accessible inside the *arguments* array
  - Offsets:
    - 0x00    : *arguments* field
    - 0x01    : arg1
    - 0x0n    : argn
    - 0x0n+1  : lvar 1
    - 0x0n+2  : lvar 2
    - 0x0n+n  : lvar n
      - $0 is rewritten to whatever the first argument is
      - $1 is rewritten to whatever the second argument is
      - in $n, if n is bigger than the method argc, it gets rewritten to *arguments[n]*

# Make sure the JIT compiler doesn't need to allocate anything via the VM
- How are instructionblocks allocated?
  - Remove the handling from the VM into a separate thing

# Instruction to add a local variable slot to the current frame's environment
- Needed to support a REPL
- Might be useful for other things?

# Think about the pros/cons of switching to NAN-boxing
- See: https://en.wikipedia.org/wiki/IEEE_754-1985#NaN
- See: https://wingolog.org/archives/2011/05/18/value-representation-in-javascript-implementations
- See: https://leonardschuetz.ch/resources/documents/typeinfo.pdf
- Floats can be encoded at compile-time not at runtime

# Rethink symbols
- Symbols should be a direct omni-directional mapping between an immediate-encoded `VALUE` and a string
- We should be able to create new symbols without an instance of the machine
  - Have constant symbols available for some commonly used strings
  - The vm needs to be initialized with a set of symbols
  - If a symbol wasn't found the string "null" is returned
- Symbols aren't available in the Charly programming language. The are merely an implementation detail
- How does the VM know about the string representation of symbols?

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
- Inject basic classes at machine startup
  - Keep a reference to these classes somewhere?

# Class Construction
- Class is called
- Set the object's klass field to the class that's used to construct it
- Insert all the classes fields and methods into the object
  - Skip the constructor method
- Check if there is a constructor inside the class
  - Check if there are enough arguments for the constructor
  - Setup an exception handler which deallocates anything if the constructor throws an exception
  - Call the constructor, setting the self value to the newly created object
- If the constructor succeeded, push the newly created object onto the stack

# Objects
- Hashes are just objects with their klass field to the Object class
- Property lookups
  - Logic should be moved into the Object class
  - The object doesn't need anything from the VM to be able to lookup a symbol (duh, thanks max)
  - If the object itself doesn't contain the property the objects parent class is checked
  - If the whole hierarchy doesn't contain the symbol, null is returned

# Strings
- Compilation
  - Strings get encoded directly into the instruction stream
    - They get copied into a string object by the vm
    - Pro:
      - Easier to compile
      - Easier to garbage-collect
      - Easier to execute
    - Sub:
      - Big strings = Big instructionblocks
      - Hard to share strings

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
  - The compiler rounds up string memory allocation to the next higher power of two
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
- Standard exception class
  - message
    - Simple string describing what the exception is about
  - trace
    - Array of strings being a dump of the frame hierarchy at the point where
      the exception was thrown.
  - Ability to rethrow exceptions

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

# Operators
- Maybe use templates for this?
  - Copy the operator matrix from the Crystal Charly source. Don't reinvent the wheel.

# Symbols
- Add a method which allows to turn arbitary types of things into symbols

# Implement all opcodes
- PutString
- PutArray
- PutClass
- ReadMemberValue
- SetMemberValue
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
