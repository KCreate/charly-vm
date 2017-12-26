/*
 The purpose of this file is to test if the parser can correctly
 parse every possible syntax of the language
*/

// Local initialisations
let a
let c = 1
const e = 1

// If statements
if a a
if (a) a
if a { a }
if a { a } else a
if a { a } else { a }
if a { a } else if a a

// Unless statements
unless a a
unless (a) a
unless a { a }
unless a { a } else a
unless a { a } else { a }

// Guard statements
guard a a
guard (a) a
guard a { a }

// While statements
while a a
while (a) a
while a { a }

// Until statements
until a a
until (a) a
until a { a }

// Loop statements
loop a
loop { a }

// Try catch
try { a } catch(e) { a }
try { a } catch(e) { a } finally { a }
try { a } finally { a }
try { a } finally { a }

// Switch statements
switch a {}
switch (a) {}
switch a {
  case 1 a
  case 1, 2 a
  case (1) a
  case (1, 2) a
  case 1 { a }
  case 1 break
  default break
}
switch a { default { a } }

// Functions
func { a }
func (a) { a }
func _myfunc1(a) { a }
func _myfunc2(a) = a
func _myfunc3(a) = return a

// Arrow functions
->a
->{ a }
->(a) a
->(a) { a }

// Control flow keywords
loop break
loop continue
loop { break }
loop { continue }
->{ return }
->{ return a }
->{ throw a }

// Assignments
a = 1
a.a = 1
a[a] = 1

// AND assignments
a += 1
a.a += 1
a[a] += 1

// Ternary ifs
a ? 1 : 2

// Operators
a + 1;
a - 1;
a * 1;
a / 1;
a % 1;
a ** 1;

// Comparison operators
a < 1;
a <= 1;
a > 1;
a >= 1;
a == 1;
a ! 1;

// Bitwise operators
a << 1;
a >> 1;
a & 1;
a | 1;
a ^ 1;

// Unary operators
+a;
-a;
!a;
~a;

// Typeof
typeof a;

// Member and call operations
a();
a.a();
a[a]();
a[a];

// Literals
@"identifier"
@a
self
a;
(1 + 1)
25
25.25
"hello world"
true
false
null
NaN;
[1, 2, 3];
{ a: 1, b: 2 };

// Classes
class {}
class _class1 {}
class _class2 extends _class1 {}
class _class3 extends _class1, _class2 {}
class _class4 extends 1 + 1 {}
class _class5 extends a(), a() {}
class _class6 extends a, a, a {}
class {
  property foo
  static property foo
  func bar {}
  static func bar {}
  func constructor {}
}
