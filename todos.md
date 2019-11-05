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
