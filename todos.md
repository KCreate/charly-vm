# Todos

- AST helper methods
  - Inline functional `as` method (e.g. `block->statements[0]->as<Int>()`)
    - Currently some issue with bad_weak_ptr exceptions and shared_ptrs
    - Happens inside shared_from_this() when there are no previous shared ptrs to this pointer

- Replace utils::vector, map, queue with regular std:: usage

- Match methods to check for AST structure and extract nodes from the graph

- Parser unit tests

- Parser
  - No desugaring or optimizations
    - These transformations are performed in the next layer
  - parse comparison operators
  - parse bitwise operators
  - parse arithmetic operators
  - parse assignment operators
  - parse control keywords (return, break, continue, defer, throw)
  - parse expression keywords (yield, spawn, import, await, typeof)
  - parse ternary expressions
  - parse lists
  - parse dicts
  - parse sets
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
  - parse defer
  - parse spawn
  - parse list destructuring assignment
  - parse object destructuring assignment
  - some illegal syntaxes can be caught by sometimes parsing newline tokens
    - inside subscript, call parsing methods

- Semantic constraints
  - Desugaring passes
  - Constant checking
  - Legal keywords
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
