# Todos

- token reader
  - parse operators
  - parse AND operators
  - string interpolation

- Parser
  - some illegal syntaxes can be caught by sometimes parsing newline tokens
    - inside subscript, call parsing methods

- Build AST representation
  - Easy to build by hand
  - Editable
  - Semantic annotations
    - Easily dump-able
  - Location information
  - Easily traversable
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
