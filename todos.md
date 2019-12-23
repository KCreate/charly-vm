- Rename `lstrip` and `rstrip` methods to some other more correct name
  - These methods should remove whitespace characters from the left or right side of a string
  - Add a `strip` method, which is equivalent to calling `rstrip(lstrip(<string>))`

- Instruction Pointer to filename & line number mapping for better stack traces
  - Create utility functions that return various location information of the call chain
    This is needed to completet the import system as some helper functions inside the prelude
    need to know from which file an import was requested.

- `super` syntax sugar to call the parent class version of a function

- do panic if some vm methods are called not from the main thread

- locale handling?
  - localization framework
  - time methods need localisation

- calendar time operations
  - add a day, add a year, add 20 years
    - should automatically calculate leap days and stuff like that

- fix exceptions on deferred functions
  - declare a global exception handler in the charly object that gets
    called whenever there is no exception handler declared
      - also useful for thrown that are running in the top frame
        as there is no exception handler there too and it always results
        in some ugly looking error messages

- Improved Garbage Collector
  - Implement a fully fledged garbage collector that has the ability to crawl
    the call tree, inspect the machine stack and search for VALUE pointers (pointers
    pointing into the gc heap area)
- Colorize methods to generate escape codes for terminals

- Rewrite the symbol system
  - Implement a method that turns arbitrary types of values into symbols
    and stores them in the global symboltable
    - Use the to_s function to convert the value to a string and then turn that into a symbol

- Rewrite the class system
  - `super` method needs to be supported
  - Multiple inheritance
  - Default values for member properties
