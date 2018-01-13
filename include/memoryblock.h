/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 Leonard Schütz
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

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#pragma once

namespace Charly {

class MemoryBlock {
  static constexpr const size_t kInitialCapacity = 64;
  static constexpr const float kGrowthFactor = 2;

public:
  MemoryBlock() {
    this->data = reinterpret_cast<uint8_t*>(std::malloc(kInitialCapacity));
    this->capacity = kInitialCapacity;
    this->writeoffset = 0;
  }
  ~MemoryBlock() {
    if (this->data) {
      std::free(this->data);
    }
  }
  MemoryBlock(const MemoryBlock& other) : data(nullptr) {
    this->data = reinterpret_cast<uint8_t*>(std::malloc(other.capacity));
    std::memcpy(this->data, other.data, other.capacity);
    this->capacity = other.capacity;
    this->writeoffset = other.writeoffset;
  }
  MemoryBlock(MemoryBlock&& other) : data(nullptr) {
    this->data = other.data;
    this->capacity = other.capacity;
    this->writeoffset = other.writeoffset;
    other.data = nullptr;
    other.capacity = 0;
    other.writeoffset = 0;
  }
  MemoryBlock& operator=(const MemoryBlock& other) {
    if (this != &other) {
      if (this->data) {
        std::free(this->data);
      }
      this->data = reinterpret_cast<uint8_t*>(std::malloc(other.capacity));
      std::memcpy(this->data, other.data, other.capacity);
      this->capacity = other.capacity;
      this->writeoffset = other.writeoffset;
    }

    return *this;
  }
  MemoryBlock& operator=(MemoryBlock&& other) {
    if (this != &other) {
      if (this->data) {
        std::free(this->data);
      }

      this->data = other.data;
      this->capacity = other.capacity;
      this->writeoffset = other.writeoffset;

      other.data = nullptr;
      other.capacity = 0;
      other.writeoffset = 0;
    }

    return *this;
  }

  // Grows the internal buffer to hold an amount of data
  inline void grow_to_fit(size_t size) {
    if (this->capacity < size) {
      // Calculate the new size
      size_t new_size = this->capacity;
      while (new_size < size) {
        new_size *= kGrowthFactor;
      }

      // Realloc the data to fit the new size
      this->data = reinterpret_cast<uint8_t*>(std::realloc(this->data, new_size));
      this->capacity = new_size;
    }
  }

  // Append data to the end of the internal buffer
  // Automatically grows the buffer to fit
  template <typename T>
  inline uint32_t write(const T& value) {
    this->grow_to_fit(this->writeoffset + sizeof(T));
    *reinterpret_cast<T*>(this->data + this->writeoffset) = value;
    this->writeoffset += sizeof(T);
    return sizeof(T);
  }

  // Shorthand methods for some commonly used types
  inline uint32_t write_u8(uint8_t val) {
    return this->write(val);
  }
  inline uint32_t write_u16(uint16_t val) {
    return this->write(val);
  }
  inline uint32_t write_u32(uint32_t val) {
    return this->write(val);
  }
  inline uint32_t write_u64(uint64_t val) {
    return this->write(val);
  }
  inline uint32_t write_double(float val) {
    return this->write(val);
  }
  inline uint32_t write_double(double val) {
    return this->write(val);
  }
  inline uint32_t write_ptr(uintptr_t val) {
    return this->write(val);
  }

  // Write data to a given offset of the internal buffer
  // Automatically grows the buffer to fit
  template <typename T>
  inline uint32_t write(const T& value, uint32_t offset) {
    this->grow_to_fit(offset + sizeof(T));
    *reinterpret_cast<T*>(this->data + offset) = value;
    return sizeof(T);
  }

  // Write a block of memory into the internal buffer
  inline uint32_t write_block(uint8_t* data, size_t size) {
    this->grow_to_fit(this->writeoffset + size);
    memcpy(this->data + this->writeoffset, data, size);
    this->writeoffset += size;
    return size;
  }

  // Write a string into the internal buffer
  inline uint32_t write_string(const std::string& data) {
    this->grow_to_fit(this->writeoffset + data.size());
    memcpy(this->data + this->writeoffset, data.data(), data.size());
    this->writeoffset += data.size();
    return data.size();
  }

  // Read data from a given offset
  template <typename T>
  inline T& read(uint32_t offset) {
    return *reinterpret_cast<T*>(this->data + offset);
  }

  // Returns a pointer to the first byte of the internal buffer
  inline uint8_t* get_data() {
    return this->data;
  }

  // Returns the current writeoffset into the internal buffer
  inline size_t get_writeoffset() {
    return this->writeoffset;
  }

protected:
  uint8_t* data;
  size_t capacity = kInitialCapacity;
  uint32_t writeoffset = 0;
};
}  // namespace Charly
