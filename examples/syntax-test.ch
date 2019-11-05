/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2019 Leonard Schütz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

// Do-While statements
do a while a
do a; while a
do { a } while (a)

// Do-Until statements
do a until a
do a; until a
do { a } until (a)

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
func (a) = a
func _myfunc1(a) { a }
func _myfunc2(a) = a
func _myfunc3(a) = return a

// Ignoring const assignment
const _myconst = 25
ignoreconst {
  _myconst = 25
}

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
a &= 1
a |= 1
a ^= 1
a <<= 1
a >>= 1

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

// Allow keywords as identifiers in some places
{ continue: 25, while: 20, let: 45 };
func ____test1() { return @break + @let + @property }
func ____test2(a) { return a.return + a.throw + a.loop + a.try }

// Classes
class {}
class _class1 {}
class _class2 extends _class1 {}
class extends _class2 {}
class _class4 extends 1 + 1 {}
class _class5 extends a() {}
class _member_init {
  property a
  property b

  func constructor(@a, @b) = null
}
class {
  property foo
  static property foo
  func bar {}
  static func bar {}
  func constructor {}
}
