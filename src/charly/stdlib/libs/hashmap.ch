/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2026 Leonard Schütz
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

class HashMapEntry {
    property key
    property hash
    property value
}

export class HashMap {
    private property bins_count
    private property bins

    func constructor {
        self.bins_count = 128
        self.bins = List.create_with(self.bins_count, ->[])
    }

    func set(key, value) {
        assert key instanceof String
        const hash = self.key_to_hash(key)
        const bin_index = self.hash_to_bin_index(hash)
        const bin = self.bins[bin_index]
        const entry = self.find_entry_in_bin(bin, key, hash)
        if entry instanceof HashMapEntry {
            entry.value = value
            return self
        }

        bin.push(HashMapEntry(key, hash, value))
        self
    }

    func set_if_not_exist(key, value) {
        assert key instanceof String
        if !self.contains(key) self.set(key, value)
    }

    func at(key) {
        assert key instanceof String
        const hash = self.key_to_hash(key)
        const bin_index = self.hash_to_bin_index(hash)
        const bin = self.bins[bin_index]
        const entry = self.find_entry_in_bin(bin, key, hash)
        if entry == null return null
        entry.value
    }

    func contains(key) {
        assert key instanceof String
        const hash = self.key_to_hash(key)
        const bin_index = self.hash_to_bin_index(hash)
        const bin = self.bins[bin_index]
        self.find_entry_in_bin(bin, key, hash) != null
    }

    func remove(key) {
        assert key instanceof String
        const hash = self.key_to_hash(key)
        const bin_index = self.hash_to_bin_index(hash)
        const bin = self.bins[bin_index]
        const index = self.find_entry_index_in_bin(bin, key, hash)
        if index == null return false
        bin.erase(index, 1)
        true
    }

    func entries = self.bins.flatten()
    func keys = self.entries().map(->(entry) entry.key)
    func values = self.entries().map(->(entry) entry.value)
    func each(...args) = self.entries().each(...args)
    func map(...args) = self.entries().map(...args)
    func size = self.entries().length
    func empty = self.size() == 0
    func notEmpty = self.size() > 0

    private func key_to_hash(key) = key.hashcode

    private func hash_to_bin_index(hash) = hash % self.bins_count

    private func find_entry_in_bin(bin, key, hash) {
        bin.findBy(->(entry) {
            entry.hash == hash && entry.key == key
        })
    }

    private func find_entry_index_in_bin(bin, key, hash) {
        bin.findIndexBy(->(entry) {
            entry.hash == hash && entry.key == key
        })
    }
}
