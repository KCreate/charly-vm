/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2020 Leonard Schütz
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

// The overall goal of this change is to rewrite all the import logic
// inside charly. This will require a finished IO and Paths library.
// The vm itself should have no idea about importing, it only
// provides the needed syntax-sugar.

// Different types of import statements:
//
// - Absolute paths
// - Relative paths
// - Library
// - Library subcomponent

// Absolute paths
import "/some/path/somelib.ch" as SomeLib
import "~/libs/bar.ch" as Bar

// Relative paths
import "renderer/engine.ch" as Engine
import "renderer/engine" as Engine      // No .ch extension required
import "file.ch" as MyLibrary

// Libraries
//
// To lookup the library, the name will be converted to all lowercase characters
// The library and subcomponent fields of the import statement are not regular identifiers,
// but have a bit more strict syntax.
//
// They can only consist of the following characters:
// - Lowercase characters
// - Uppercase characters
// - Digits
// - '_'
//
// Furthermore, a library or component identifier may only begin with a character (lower- or uppercase)
import GFXLib

// Library subcomponent
//
// When loading a subcomponent from a library, only that
// specific part of the library will be loaded. The result will
// then be stored inside the corresponding component identifier.
import Lib::Graphics
import Lib::Audio
import Lib::Keyboard

// Only a single subcomponent is allowed
//
//                  +-- Illegal DoubleColon token
//                  |
//                  v
import Lib::Graphics::Foo

// All import statements can specify a symbol in which the loaded
// library will be stored.
import FooLib as SomeOtherName
import Datastructures::MinHeap as Heap

// Imports that are prefixed with '::' import built in charly libraries
import IO::FileSystem
import IO::Networking

// You can specify symbols to be extracted from the library
// If only a single symbol is needed, the curly braces can be omitted
import SomeComponent from SomeLib
import { Reader, Writer, Encoder } from SomeLib::Encryption

// The following statement would load SomeLib, extract symbols foo, bar and baz
// and store them in a constant called Bar
import { foo, bar, baz } as Bar from SomeLib

// The import statement is simply syntax sugar and gets compiled down
// to the following code
//
// The following bitmasks tell the import algorithm some information
// about where to find the requested resource
//
//  Original code                                     De-sugared code
    import "foo.ch" as Foo                            const Foo          = Charly.import.load_path("foo.ch")

    import SomeLib                                    const SomeLib      = Charly.import.load_lib(["SomeLib"])

    import SomeLib as OtherLib                        const OtherLib     = Charly.import.load_lib(["SomeLib"])

    import SomeLib::Sub                               const Sub          = Charly.import.load_lib(["SomeLib", "Sub"])

    import SomeLib::Sub as Foo                        const Foo          = Charly.import.load_lib(["SomeLib", "Sub"])

    import Foo from SomeLib                           const { Foo }      = Charly.import.load_lib(["SomeLib"])

    import { Foo, Bar } from SomeLib                  const { Foo, Bar } = Charly.import.load_lib(["SomeLib"])

    import { Foo, Bar } as Lib from SomeLib           const Lib = null
                                                      ignoreconst {
                                                        const { Foo, Bar } = Charly.import.load_lib(["SomeLib"])
                                                        Lib = { Foo, Bar }
                                                      }

// Every charly library needs to contain a package.ch file
// This file simply exports an object with some pre-defined keys
export {

  // The name of this library
  name: "MyMediaLibrary",

  // Version identifier in the format of Major, Minor, Patch
  version: "10.15.3",

  // Information about the repository where this library is hosted at
  repository: { type: "git", url: "git://github.com/octocat/my-media-library" }

  // Author information
  author: {
    name: "Leonard Schütz",
    email: "leni.schuetz@me.com",
    social: {
      github: "KCreate"
    }
  }

  // This file gets loaded when you import the entire library.
  main: "src/main.ch",

  // Loading a sub-component of a library will lookup
  // in this table and load the corresponding file
  components: [
    Audio: "src/components/audio/main.ch",
    Graphics: "src/components/graphics/main.ch",
    Keyboard: "src/components/keyboard/main.ch",
    Network: "src/components/network/main.ch"
  ],

  // List of dependencies this library depends on. This is useful
  // if i want to implement a package manager in the future.
  // The exact format has been left empty, I will think about this
  // once the time has come.
  dependencies: [...]
}
