# Todos

- token reader
  - parse structure tokens
  - parse comments
  - string interpolation

- Parser
  - some illegal syntaxes can be caught by sometimes parsing newline tokens
    - inside subscript, call parsing methods

- Build AST representation
  - No boilerplate code for traversal and modification visitors
    - Visitor classes should just be able to define certain virtual methods
    - Somehow automatically generate these methods?
  - Easy to build by hand
  - Semantic annotations
  - Location information

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
