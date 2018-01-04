# Todos

# Class parsing
- Check for duplicate member and static property declarations
- Check for duplicate member and static functions

# Add CallIsolated instruction
- Calls a function with setting the parent_environment_frame field

# Switch to C++14 as most distributions don't have a C++17 compiler installed
- Requiring users to build one themselves isn't cool
- Requiring users to add a PPA isn't cool either

# VM instruction block handling
- How does the VM start execution of a new instruction block
  - Call `VALUE VM::run_block` to execute a block
  - The VM treats the instructionblock as the body of a function
    - VM pushes Charly and an empty export object and calls the new instructionblock
    - Once the VM halts, it knows that it reached the end of this instructionblock and returns from run_block
- How does the VM know in which instruction block it currently is in
  - New design
    - Frames don't need the function argument, it is optional
      - It is only used for stacktraces, pretty-printing
      - Stacktrace entries without functions contain the address
    - Running a new instruction block requires creating a frame for it and setting the function property to `nullptr`

# Dynamic lookup of symbols?
- Makes a REPL trivial
- Easy to turn an object into a stackframes environment table, and back
- Easy implementation of global symbols
- What needs to be changed?
  - Stack frames would need a mapping from symbols to offsets
    - `std::map<Symbol, Offset>`
  - New opcodes
    - readdynamic symbol
    - setdynamic symbol

# VM bootstrapping
- On startup, the VM runs a prelude file
  - Loads some libraries
  - Loads some internal methods
  - Creates some shorthand bindings to commonly used methods such as
  - Registers some file descriptors such as `io.stdout`, and `io.stdin`
  - `require` can be implemented like this:
    ```javascript
    const Charly = {}; // global Charly objects
    const require = func require(name) {

      // Global variables accessible to every module
      const module_context = {
        require_no_exec,
        require,
        Object,
        Class,
        Array,
        String,
        Number,
        Function,
        Boolean,
        Null,
        stdin: io.stdin,
        stdout: io.stdout,
        stderr: io.stderr,
        print: io.print.stdout,
        write: io.write.stdout,
        gets: io.gets,
        getc: io.getc,
        exit: io.exit,
        sleep: io.sleep
      };

      // Load the module without executing it
      return module_context.require_no_exec(name)(Charly, {});
    }
    ```
  - VM has a method `__require_no_exec` which allows to include a file
    - It compiles and returns the included file as a function that can be called
    - It injects as arguments the following values from the prelude scope:
      - Charly
      - export
      - require
      - Object
      - Class
      - Array
      - String
      - Number
      - Function
      - Boolean
      - Null
      - stdin
      - stdout
      - stderr
      - print
      - puts
      - write
      - gets
      - getc
      - exit
      - sleep
      - Exception

# VM require
- How does the VM include a file?
- Reuse the same parser instance or createa a new one?
- Who knows about and keeps track of InstructionBlocks
  - An external class should do this
    - We don't want to recompile a file every single time it is run
      - Store a hash of a file and the compiled result
      - If the hashes match, return the compiled result
      - If the hashes don't match, recompile the program and add new entry

# Move language logic which doesn't depend on the VM into it's own class
- Maybe called `VMUtils` or `CharlyUtils`?
  - Needs access to the context
- One doesn't need the VM to pretty_print objects
- The VM should only contain logic that is stricly specific to executing code
- Candidate methods
  - `create_instructionblock`
  - `create_object`
  - `create_array`
  - `create_float`
  - `create_string`
  - `create_cfunction`
  - `cast_to_double`
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

# Extending primitive types
- How do you extend primitive types?
- How does the VM lookup these symbols?
  - Table with references to classes defined in a prelude?
  - Fixed offsets inside top frame?

# GC Optimizations
- GC should try to reorder the singly-linked list of free cells so that cells which
  are located beneath each other inside memory should also be linked directly
  - This allows for optimized arrays as we would not need a vector anymore,
    just two pointers. Push and pop might also benefit from this.

# Exception safety in compiler
- Allocated AST nodes need to be deallocated when an exception is thrown
- unique_ptr might do what I want
- An exception means a syntax error
  - If a syntax error occurs, the whole program should crash.
  - This means it's not as bad if we loose memory if we're going to crash anyway
  - Not an excuse, but reduces it's priority to be fixed
- Template schenanigans could register AST nodes in a pool which would be cleared if an exception occurs

# Extra syntax for variadic functions
- Only insert the `arguments` array into functions which need it.
  - Special syntax for functions that receive a variable amount of arguments
    - `func foo(a, b)[args] { ... }`
      - Pros:
        - Easy parsing
      - Cons:
        - Looks ugly
        - Easily confused with other syntaxes
        - Too much syntax for little functionality
    - `func foo(a, b, args...) { ... }`
      - Pros:
        - Looks good
        - Easy understood as it looks the same in other languages
      - Cons:
        - If the variadic entry is in the middle
          the offsets to the latter arguments are not known at compile-time
          - Generate `ReadArrayIndex` instructions for dynamic arg identifiers
          - Generate `SetArrayIndex` for assignments to dynamic arg identifiers
        - More complicated parsing and semantic validation
  - The regular arglist gives names to the first couple arguments
    - Determines the methods required argument count
  - If a function doesn't have the variadic notation, it is marked as non-variadic
    - If a function is not variadic, the VM doesn't have to create the arguments array

# Compiler
- Clean the code of the compiler
  - Come up with a good way to structure all the things the compiler does and produces
    - Errors
    - Warnings
    - Results
      - Produced symbols
      - InstructionBlocks
      - Static data (strings)
    - Logging
- Move illegal keyword checking from the parser to the normalizer

# Remove PutCFunction instruction
- Runtime shouldn't be able to create functions to arbitrary addresses (security risk)
- Only the VM should be able to create CFunctions

# `super` inside class instance methods
- Checks the self value
- If self is an object, check the klass value
- Check each parent if they have an instance method with the same name as the original function
  - The first result will be returned
- Needs new instructions
  - `CallSuper`
- Class should have a separate field for the constructor function
  - Update opcodes to accept an optional constructor

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
- `Charly.Numeric`, `Charly.String`, etc.

# Class Construction
- Parent class list is uniqified in the PutClass instruction
- Class is called
- Set the object's klass field to the class that's used to construct it
- Insert all the classes fields and methods into the object
- Check if there is a constructor inside the class
  - Check if there are enough arguments for the constructor
  - Setup an exception handler which deallocates anything if the constructor throws an exception
  - Call the constructor, setting the self value to the newly created object
- If the constructor succeeded, push the newly created object onto the stack

# Objects
- Hashes are just objects with their klass field to the Object class
- Property lookups
  - The object doesn't need anything from the VM to be able to lookup a symbol
    - Logic should still be contained inside the VM class
  - If the object itself doesn't contain the property the objects parent class is checked
  - If the whole hierarchy doesn't contain the symbol, null is returned

# Mapping between JITed code and source file locations
- Mapping between a range of instructions to a range of row/col pairs on compilation?
  - Runtime exceptions can use this information to show a pretty error screen on crash
- `InstructionBlock` should have a filename property
  - Blocks created at runtime or during compilation which do not directly map to a file
    should be called `VM:n` where n is a globally increasing integer starting at 0
- InstructionBlock has an optional property of debug information
  - Take inspiration from real debug information formats
  - Mapping betweens offset ranges and row/column pairs

# Exception system
- Standard exception class
  - message
    - Simple string describing what the exception is about
  - trace
    - Array of trace entries containing the function, file, row, column of the location
      from where the exception was thrown

# Define deconstructors for some objects
- If we manage all resources in pre-defined types, we won't need a "finalize" function
  - CFunctions might allocate stuff and the deallocation logic might be written inside Charly
  - Maybe provide enough data structures inside the vm, so that deallocation logic inside
    Charly can be mitigated, thus leaving all dealloc logic to the GC.

    This could be achieved by wrapping pointers (maybe?). Thus, C-Bindings would return
    wrapped pointers, optionally defining a function pointer for a destructor function.
    We can keep the correct context object by using the `->*` and `.*` operators.
    See: https://isocpp.org/wiki/faq/pointers-to-members#dotstar-vs-arrowstar

# Debugging info
- Use `#ifdef` preprocessor commands to create debug and profiling fields inside some structures
  of the VM.
  - Function could contain the plaintext names of the arguments and local variables

# Operators
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
