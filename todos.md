# Todos

# Figure out how the VM starts execution of a block
- Exactly the starting and end point is interesting
- Maybe pass a pointer to an InstructionBlock?
- What happens when the VM halts? (via the Halt instruction)
- How does the VM give back control to the calling function
- How is VM exit being done

# Move language logic which doesn't depend on the VM into it's own class
- Maybe called `VMUtils` or `CharlyUtils`?
  - Needs access to the context
- One doesn't need the VM to pretty_print objects
- The VM should only contain logic that is stricly specific to executing code
- Candidate methods
  - `create_instructionblock`
  - `create_object`
  - `create_array`
  - `create_integer`
  - `create_float`
  - `create_string`
  - `create_cfunction`
  - `cast_to_double`
  - `integer_value`
  - `float_value`
  - `numeric_value`
  - `boolean_value`
  - `real_type`
  - `type`
  - `pretty_print` (Requires access to the symbol_table and pretty_print_stack and VM data structures)

# Instruction to add a local variable slot to the current frame's environment
- Needed to support a REPL
- Might be useful for other things?
- Does this require a modification of the InstructionBlock structure?
- The compiler needs to keep around semantic information for the whole program

# Calling and object storage convention
- `arguments`
  - `__CHARLY_FUNCTION_ARGUMENTS` should be a reserved identifier
  - `__CHARLY_FUNCTION_ARGUMENTS` will be rewritten to `ReadLocal 0, 0`
  - `arguments` will also be rewritten to `ReadLocal 0, 0`
    - If the user redeclares his own `arguments` lvar, it will be treated as a new lvar
- $0, $1, $2, $n get rewritten to
  ```
  ReadLocal 0, 0
  PutValue $n
  ReadMemberValue
  ```
- Where does the return value end up?
  - Callee side:
    - The return value of the call is the top value on the stack
  - Caller side:
    - Pushing it onto the stack could work
      - Compiler would have to make sure there is nothing else on the stack
    - Dedicated return_value field inside current frame
      - Pushed onto the stack on frame pop

# Execution pipeline
- Managed by a single `Context` class.
- Contains a `SymbolTable` and `MemoryManager`
- `Compiler`
  - Takes source code as input and outputs charly bytecodes inside an InstructionBlock
    - Produces a symbol table with all the symbols created during compilation
    - Produces a string pool containing all strings of the source code
    - Produces a source map mapping different instructions to offsets in the source file
  - Tries to optimize the code
    - Optimized code isn't a goal but simple peep-hole optimisations can still be done
- Data should be creatable without the need of a VM object
  - Some data types are inherently dependent on the VM, such as the `Function` type
- The VM should be purely used for execution, not data creation

# GC Optimizations
- GC should try to reorder the singly-linked list of free cells so that cells which
  are located beneath each other inside memory should also be linked directly
  - This allows for optimized arrays as we would not need a vector anymore,
    just two pointers. Push and pop might also benefit from this.

# AST Implementation
- I want to avoid the use of dynamic_cast as much as possible
  - This is not a performance concern, but ease-of-use
  - dynamic_cast is super long to write and looks rather ugly
- typeid can be used.
  - This works, constants have been defined inside ast.h
- Function overloading based on argument type doesn't work
  - Needs a dispatch function which performs a typecheck and redirects to correct method
    - Maybe C++ offers a way this can be implemented natively

# Compiler
- _LVarRewriter_: Semantic (undefined and duplicate lvars) and lvar offset calculation in one step
  - Buffers 5 errors
    - Throws if more than 5 errors were generated
  - Calculates how many local variable slots a function frame will need

# RemoveConstant instruction
- Needed to implemented proper lifetime of const local variables inside child blocks

# Make sure the JIT compiler doesn't need to allocate anything via the VM
- How are instructionblocks allocated?
  - Remove the handling from the VM into a separate thing

# Refactoring and code structure
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

# Reserved identifiers
- Everywhere
  - `self`
- Only top-level context
  - `ARGV`
  - `IFLAGS`
  - `ENV`
  - `Charly`
- Inside functions
  - `__CHARLY_FUNCTION_ARGUMENTS`

# Think about the pros/cons of switching to NAN-boxing
- See: https://en.wikipedia.org/wiki/IEEE_754-1985#NaN
- See: https://wingolog.org/archives/2011/05/18/value-representation-in-javascript-implementations
- See: https://leonardschuetz.ch/resources/documents/typeinfo.pdf
- Floats can be encoded at compile-time not at runtime

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

# Memory ownership
- Structs generally own the memory they point to
- InstructionBlocks
  - IB's are owned by the function they are placed into
  - Once the function is being deallocated, their instructionblock is deallocated as well

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
        - This should never happen and can be catched at an early level during parsing and semantic validation
- Standard exception class
  - message
    - Simple string describing what the exception is about
  - trace
    - Array of strings being a dump of the frame hierarchy at the point where
      the exception was thrown.

# Define deconstructors for some objects
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

# Implement conversion methods
- `cast_to_numeric` Convert value type to numeric or NAN if not possible
- `cast_to_integer` Convert value type to integer or NAN if not possible

# Implement all opcodes
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
