#include <stdio.h>

typedef long VALUE;

#define RUBY_BIT_ROTL(v, n) (((v) << (n)) | ((v) >> ((sizeof(v) * 8) - n)))
#define RUBY_BIT_ROTR(v, n) (((v) >> (n)) | ((v) << ((sizeof(v) * 8) - n)))

static inline double
rb_float_flonum_value(VALUE v) {

  if (v != (VALUE)0x8000000000000002) {
    union {
      double d;
      VALUE v;
    } t;

    VALUE b63 = (v >> 63);

    t.v = RUBY_BIT_ROTR((2 - b63) | (v & ~(VALUE)0x03), 3);

    return t.d;
  }

  return 0.0;
}

static inline VALUE
rb_float_new_inline(double d) {
    union {
      double d;
      VALUE v;
    } t;

    int bits;

    t.d = d;
    bits = (int)((VALUE)(t.v >> 60) & 0x7);

    if (t.v != 0x3000000000000000 && !((bits-3) & ~0x01)) {
      return (RUBY_BIT_ROTL(t.v, 3) & ~(VALUE)0x01) | 0x02;
    } else if (t.v == (VALUE)0) {
      /* +0.0 */
      return 0x8000000000000002;
    }

    return (VALUE)0;
}

int main() {

  VALUE val1 = rb_float_new_inline(5.5);
  VALUE val2 = rb_float_new_inline(5.9);
  VALUE val3 = rb_float_new_inline(1000.577);

  double db1 = rb_float_flonum_value(val1);
  double db2 = rb_float_flonum_value(val2);
  double db3 = rb_float_flonum_value(val3);

  printf("%f\n", db1);
  printf("%f\n", db2);
  printf("%f\n", db3);

  return 0;
}
