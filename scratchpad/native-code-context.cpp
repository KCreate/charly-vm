/*
 * Context becomes a container for the arguments and result / error values.
 * C functions can also access the thread mode system via the context.
 * Allocations of Charly values are also performed via the Context
 * */
class Context {
public:
  Context* error();
  Context* error(const char* message);
  Context* error(const std::string& message);

  Context* finish();
  Context* finish(void* value);
  Context* finish(int value);
  Context* finish(double value);
  Context* finish(VALUE value);

  template <class T, typename... Args>
  __attribute__((always_inline))
  inline T* allocate(Args&&... params) {
    Coordinator::assert_thread_state(ThreadState::Charly);
    return charly_allocate<T>(std::forward<Args>(params)...);
  }

private:
  VALUE* argv;
  uint32_t argc;
  uint32_t argi;
};

#define ARG(T, N, CT) T N; { if (!ctx.arg(&N, CT)) return ctx; }

Context* buf_write(Context* ctx) {

  // Automatic argument parsing
  // Returns an error with detailed error message
  //
  // ARG statements need to be in order that they are passed
  // to the function in Charly
  ARG(Buffer*, buf,     TypePointer);
  ARG(String*, message, TypeString);
  ARG(Object*, options, TypeObject);

  // Returning custom errors
  if (message->length >= 256) {
    return ctx.error("Message cannot be longer than 256 characters");
  }

  // Holding locks on charly values
  ctx.lock(options, [&] {

  })

  // Enter a native section
  ctx.native_section([&] {
    // execute code that does not depend on Charly code here
  });

  // Allocate a result object
  Immortal<Object> result = ctx.allocate<Object>();
  result->write(ctx.symbol("name"), ctx.allocate_string("Leonard"));
  result->write(ctx.symbol("age"), ctx.create_number(20));
  result->write(ctx.symbol("height"), ctx.create_number(186));
  return ctx.finish(result);
}

Context* cos(Context* ctx) {
  ARG(double, x, TypeDouble);
  double result = std::cos(x);
  return ctx.finish(result);
}
