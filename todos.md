# Todos

- Clang Tidy Setup
  - Function naming scheme?

- AST building methods
  - Constructing new AST nodes should be short and simple

- Match methods to check for AST structure and extract nodes from the graph
  - Similar to LLVMs match syntax

- Parser unit tests

- AST Visitor
  - Different tree traversal hooks
    - OnEnterBlock, OnLeaveBlock    Called when the node is entered, left
    - OnVisitBlock                  Called when the node is entered, expected to visit children manually
    - Visited node can be replaced by writing to the
      shared_ptr reference passed to the visitor callback

- Build AST representation
  - No boilerplate code for traversal and modification visitors
    - Visitor classes should just be able to define certain virtual methods
    - Somehow automatically generate these methods?
  - Easy to build by hand
  - Semantic annotations
  - Location information

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
