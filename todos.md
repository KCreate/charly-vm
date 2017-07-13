# Todos

# Internal methods system
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

# Operators
- Maybe use templates for this?
  - Copy the operator matrix from the Crystal Charly source. Don't reinvent the wheel.

# Implement all opcodes
- ReadMemberValue
- SetMemberValue
- PutString
- PutArray
- PutClass
- Pop
- Dup
- Swap
- Topn
- Setn
- CallMember
- Throw (only Return is implemented so far)
- Branch
- BranchIf
- BranchUnless
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

# Exception system
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

























------
