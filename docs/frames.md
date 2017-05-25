# Frames

Frames are used to handle function control-flow, exception handling and scoping. Each frame holds
has a type and some associated information.

## Call frames

Call frames are used to create a new procedure. They only hold a return address that denoted
where to continue execution once the procedure finishes.

## Environment frames

Environment frames are used to introduce a new level of scope. An environment frame is pushed
at the beginning of every block.

## Exception handler frame

Exception handler frames are pushed at the beginning of each exception handler. Language constructs such
as try/catch statements can be implemented this way.

## Redirection frames

Redirection frames are used introduce branching to the environment stack. The below diagram tries to
explain how redirection frames could be used.

```c
foo(25, ->(arg : int) {
  printf("argument: " + arg);
});

func foo(num : int, callback : func(int, void)) {
  callback(num);
}
```

The frame stack at the printf call would look like this:

```
(environment - global) <--+ <--+
(callframe - foo)         |    |
(redirection - global) ---+    |
(environment - foo)            |
(callframe - callback)         |
(redirection - global) --------+
(environment - callback)
```

Each cell describes a single frame. The type of the frame is on the left side and a small description
on the right side. The arrows coming from the redirection frames show to which frames they point to.

As you can see, the second redirection frame, "jumps" over the environment frames pushed by the `foo` method.
