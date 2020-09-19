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

export = ->(describe, it, assert) {
  describe("path operations", ->{
    it("creates a new path", ->{
      assert((new Path("foo.txt")).to_s(), "foo.txt")
      assert((new Path("foo/bar.txt")).to_s(), "foo/bar.txt")
      assert((new Path("/foo/bar/baz.txt")).to_s(), "/foo/bar/baz.txt")
      assert((new Path("./foo.txt")).to_s(), "./foo.txt")
      assert((new Path("../foo.txt")).to_s(), "../foo.txt")
      assert((new Path("..")).to_s(), "..")
      assert((new Path("../.././foo.txt")).to_s(), "../.././foo.txt")
      assert((new Path("~/foo.txt")).to_s(), "~/foo.txt")
      assert((new Path("////./foo/bar/baz")).to_s(), "/./foo/bar/baz")
    })

    it("appends other paths", ->{
      const base = new Path("./base")

      const cases = [
        "foo.txt",
        "./foo.txt",
        "../foo.txt",
        "~/foo.txt"
      ]

      const results = [
        "./base/foo.txt",
        "./base/./foo.txt",
        "./base/../foo.txt",
        "./base/~/foo.txt"
      ]

      cases.each(->(c, i) {
        const path = new Path(base)
        path.append(c)

        assert(path.to_s(), results[i])
      })
    })

    it("prepends other paths", ->{
      const base = new Path("data/test/foo.txt")

      const cases = [
        "/",
        "./foo",
        "../foo",
        "~/data",
        "foo",
        "foo/bar"
      ]

      const results = [
        "/data/test/foo.txt",
        "./foo/data/test/foo.txt",
        "../foo/data/test/foo.txt",
        "~/data/data/test/foo.txt",
        "foo/data/test/foo.txt",
        "foo/bar/data/test/foo.txt"
      ]

      cases.each(->(c, i) {
        const path = new Path(base)
        path.prepend(c)

        assert(path.to_s(), results[i])
      })
    })

    it("normalizes a path", ->{

      // Mock home directory
      const home_backup = ENVIRONMENT["HOME"]
      ENVIRONMENT["HOME"] = "/foo/bar/baz"

      assert(Path.normalize("foo.txt").to_s(), "foo.txt")
      assert(Path.normalize("/foo.txt").to_s(), "/foo.txt")
      assert(Path.normalize("./foo.txt").to_s(), "foo.txt")
      assert(Path.normalize("~/foo.txt").to_s(), "~/foo.txt")

      assert(Path.normalize("~/../foo.txt").to_s(), "foo.txt")
      assert(Path.normalize("../foo").to_s(), "../foo")
      assert(Path.normalize("../../foo").to_s(), "../../foo")
      assert(Path.normalize("./../../foo").to_s(), "../../foo")

      assert(Path.normalize("/../../foo").to_s(), "/foo")
      assert(Path.normalize("/../bar/foo").to_s(), "/bar/foo")
      assert(Path.normalize("/../bar/../foo").to_s(), "/foo")
      assert(Path.normalize("/foo/bar/").to_s(), "/foo/bar")

      assert(Path.normalize("/../~/foo").to_s(), "/~/foo")
      assert(Path.normalize("/~/../foo").to_s(), "/foo")
      assert(Path.normalize("///../../~/../foo").to_s(), "/foo")
      assert(Path.normalize("/../~/../foo").to_s(), "/foo")

      assert(Path.normalize("base/foo/../bar/../baz/foo.txt").to_s(), "base/baz/foo.txt")
      assert(Path.normalize("../../foo/foo.txt").to_s(), "../../foo/foo.txt")
      assert(Path.normalize("////foo.txt").to_s(), "/foo.txt")
      assert(Path.normalize("//////..////////foo.txt").to_s(), "/foo.txt")

      assert(Path.normalize("foo/////bar/./././baz.txt").to_s(), "foo/bar/baz.txt")
      assert(Path.normalize("").to_s(), ".")
      assert(Path.normalize("/foo/.").to_s(), "/foo")
      assert(Path.normalize("/foo/./../bar").to_s(), "/bar")

      assert(Path.normalize("/foo/./../bar").to_s(), "/bar")
      assert(Path.normalize("/./").to_s(), "/")
      assert(Path.normalize("/foo/.").to_s(), "/foo")
      assert(Path.normalize("/foo/../.").to_s(), "/")

      assert(Path.normalize(".").to_s(), ".")
      assert(Path.normalize("./././").to_s(), ".")
      assert(Path.normalize("/").to_s(), "/")
      assert(Path.normalize("/foo/..").to_s(), "/")

      // Restore original HOME ENV
      ENVIRONMENT["HOME"] = home_backup
    })

    it("expands a path", ->{
      assert(Path.expand("foo.txt", "/base").to_s(), "/base/foo.txt")
      assert(Path.expand("../foo.txt", "/base").to_s(), "/foo.txt")
      assert(Path.expand("../foo.txt", "/base/../foo").to_s(), "/foo.txt")
      assert(Path.expand("/foo.txt", "/base").to_s(), "/foo.txt")

      const wd = Path.getwd()
      const origin_path = new Path("foo.txt")
      const expanded_path = (new Path(origin_path)).expand(wd)
      const expected = (new Path(wd)).append(origin_path)

      assert(expanded_path.to_s(), expected.to_s())
    })

    it("removes last component from path", ->{
      assert((new Path("/foo.txt")).remove_last().to_s(), "/")
      assert((new Path(".")).remove_last().to_s(),        ".")
      assert((new Path("/")).remove_last().to_s(),        "/")
      assert((new Path("~")).remove_last().to_s(),        ".")
      assert((new Path("./foo")).remove_last().to_s(),    ".")
      assert((new Path("/foo")).remove_last().to_s(),     "/")
      assert((new Path("~/foo")).remove_last().to_s(),    "~")
      assert((new Path("/foo/")).remove_last().to_s(),    "/")
      assert((new Path("/foo/~")).remove_last().to_s(),   "/foo")
      assert((new Path("/foo///")).remove_last().to_s(),  "/")
      assert((new Path("/foo/..")).remove_last().to_s(),  "/foo")
      assert((new Path("foo/..")).remove_last().to_s(),   "foo")
      assert((new Path("~/foo/..")).remove_last().to_s(), "~/foo")
    })

    it("removes extension from path", ->{
      assert((new Path("foo")).remove_extension().to_s(), "foo")
      assert((new Path("foo.bar")).remove_extension().to_s(), "foo")
      assert((new Path("foo.bar.baz")).remove_extension().to_s(), "foo.bar")
      assert((new Path("foo.bar.baz.qux")).remove_extension().to_s(), "foo.bar.baz")

      assert((new Path("foo/bar/foo.bar.baz.qux")).remove_extension().to_s(), "foo/bar/foo.bar.baz")
      assert((new Path("/foo/bar/foo.bar.baz.qux")).remove_extension().to_s(), "/foo/bar/foo.bar.baz")

      assert((new Path(".")).remove_extension().to_s(), ".")
      assert((new Path("..")).remove_extension().to_s(), "..")
    })

    it("replaces extension of path", ->{
      assert((new Path("foo")).replace_extension("txt").to_s(), "foo")
      assert((new Path("foo.bar")).replace_extension("txt").to_s(), "foo.txt")
      assert((new Path("foo.bar.baz")).replace_extension("txt").to_s(), "foo.bar.txt")

      assert((new Path("foo/bar/foo.bar.baz.qux")).replace_extension("txt").to_s(), "foo/bar/foo.bar.baz.txt")
      assert((new Path("/foo/bar/foo.bar.baz.qux")).replace_extension("txt").to_s(), "/foo/bar/foo.bar.baz.txt")

      assert((new Path(".")).replace_extension("txt").to_s(), ".")
      assert((new Path("..")).replace_extension("txt").to_s(), "..")
    })

    it("replaces filename of path", ->{
      assert((new Path("foo")).replace_filename("bar").to_s(), "bar")
      assert((new Path("foo/bar")).replace_filename("baz").to_s(), "foo/baz")
      assert((new Path("/foo/bar/baz")).replace_filename("bar").to_s(), "/foo/bar/bar")
      assert((new Path("/test.txt")).replace_filename("bar").to_s(), "/bar")
    })
  })

  describe("path querying", ->{
    it("returns the filename of a path", ->{
      assert((new Path("foo")).filename(), "foo")
      assert((new Path("foo/bar")).filename(), "bar")
      assert((new Path("foo/bar.txt")).filename(), "bar.txt")
      assert((new Path("/foo/bar.txt")).filename(), "bar.txt")

      assert((new Path("")).filename(), ".")
      assert((new Path("/")).filename(), "/")
      assert((new Path(".")).filename(), ".")
      assert((new Path("..")).filename(), "..")
    })

    it("returns the extension of a path", ->{
      assert((new Path("foo")).extension(), "")
      assert((new Path("foo/bar")).extension(), "")
      assert((new Path("foo/bar.txt")).extension(), "txt")
      assert((new Path("/foo/bar.txt")).extension(), "txt")

      assert((new Path("")).extension(), "")
      assert((new Path("/")).extension(), "")
      assert((new Path(".")).extension(), "")
      assert((new Path("..")).extension(), "")
    })

    it("returns the stem of a path", ->{
      assert((new Path("foo")).stem(), "foo")
      assert((new Path("foo/bar")).stem(), "bar")
      assert((new Path("foo/bar.txt")).stem(), "bar")
      assert((new Path("/foo/bar.txt")).stem(), "bar")
      assert((new Path("foo/bar.baz.txt")).stem(), "bar.baz")

      assert((new Path("")).stem(), ".")
      assert((new Path("/")).stem(), "/")
      assert((new Path(".")).stem(), ".")
      assert((new Path("..")).stem(), "..")
    })

    it("returns the parent directory of a path", ->{
      assert((new Path("foo")).parent_directory().to_s(), ".")
      assert((new Path("foo/bar")).parent_directory().to_s(), "foo")
      assert((new Path("foo/bar/baz")).parent_directory().to_s(), "foo/bar")
      assert((new Path("/foo/bar/baz")).parent_directory().to_s(), "/foo/bar")

      assert((new Path("..")).parent_directory().to_s(), "../..")
      assert((new Path("foo/..")).parent_directory().to_s(), "foo/../..")
      assert((new Path(".")).parent_directory().to_s(), "./..")
      assert((new Path("/")).parent_directory().to_s(), "/")

      assert((new Path("foo/.")).parent_directory().to_s(), "foo/./..")
      assert((new Path("/..")).parent_directory().to_s(), "/../..")
    })

    it("checks wether a path is absolute", ->{
      assert((new Path("/")).is_absolute(), true)
      assert((new Path("/foo")).is_absolute(), true)
      assert((new Path("/foo/bar")).is_absolute(), true)
      assert((new Path("/.")).is_absolute(), true)
      assert((new Path("/..")).is_absolute(), true)
      assert((new Path("foo")).is_absolute(), false)
      assert((new Path("foo/bar")).is_absolute(), false)
      assert((new Path("~")).is_absolute(), false)
      assert((new Path("~/foo")).is_absolute(), false)
      assert((new Path(".")).is_absolute(), false)
      assert((new Path("..")).is_absolute(), false)

      assert((new Path("/")).is_relative(), false)
      assert((new Path("/foo")).is_relative(), false)
      assert((new Path("/foo/bar")).is_relative(), false)
      assert((new Path("/.")).is_relative(), false)
      assert((new Path("/..")).is_relative(), false)
      assert((new Path("foo")).is_relative(), true)
      assert((new Path("foo/bar")).is_relative(), true)
      assert((new Path("~")).is_relative(), true)
      assert((new Path("~/foo")).is_relative(), true)
      assert((new Path(".")).is_relative(), true)
      assert((new Path("..")).is_relative(), true)
    })

    it("checks wether a path is based on the home directory", ->{
      assert((new Path("~")).is_home(), true)
      assert((new Path("~/foo")).is_home(), true)

      assert((new Path("/~")).is_home(), false)
      assert((new Path("/~/foo")).is_home(), false)
    })

    /*it("checks wether a path begins with another path", ->{*/

    /*})*/

    /*it("checks wether a path ends with another path", ->{*/

    /*})*/

    /*it("checks wether a path contains another path", ->{*/

    /*})*/
  })

  /*describe("static path methods", ->{*/
    /*it("returns the path of the calling file", ->{*/

    /*})*/

    /*it("returns the directory of the calling file", ->{*/

    /*})*/

    /*it("returns the current working directory", ->{*/

    /*})*/

    /*it("changes the current working directory", ->{*/

    /*})*/

    /*it("returns the home directory of the user", ->{*/

    /*})*/
  /*})*/
}
