/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2021 Leonard Schütz
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
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <sstream>

class MyStream : public std::iostream, protected std::streambuf {
public:
  char* m_buffer;
  int32_t m_capacity;

  MyStream(size_t capacity) : std::iostream(this), std::streambuf(), m_buffer(nullptr), m_capacity(0) {}

  int_type underflow() override {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    if (egptr() < pptr()) {
      setg(pbase(), pbase() + (gptr() - eback()), pptr());
      return underflow();
    }

    return traits_type::eof();
  }

  int_type pbackfail(int_type __c) override {
    if (traits_type::eq_int_type(__c, traits_type::eof())) {
      if (eback() < gptr()) {
        gbump(-1);
        return traits_type::not_eof(__c);
      } else {
        return __c;
      }
    } else {
      if (eback() >= gptr()) {
        return traits_type::eof();
      }
      gbump(-1);

      if (!traits_type::eq_int_type(__c, *gptr())) {
        *gptr() = __c;
      }

      return __c;
    }
  }

  int_type overflow(int_type __c) override {
    if (traits_type::eq_int_type(__c, traits_type::eof())) {
      return 0;
    }

    if (pptr() < epptr()) {
      sputc(__c);
      return __c;
    }

    // allocate new buffer
    size_t new_capacity = m_capacity ? m_capacity * 2 : 8;
    assert(new_capacity);
    char* new_buffer = (char*)malloc(new_capacity);
    assert(new_buffer);

    std::memset(new_buffer, 'A', new_capacity);

    // copy old contents into buffer
    if (m_buffer != nullptr) {
      assert(m_buffer == pbase());
      assert(m_capacity != 0);
      traits_type::copy(new_buffer, m_buffer, pptr() - pbase());
      free(m_buffer);
    }

    m_buffer = new_buffer;
    m_capacity = new_capacity;

    size_t old_write_offset = pptr() - pbase();
    size_t old_read_offset = gptr() - egptr();
    size_t old_read_max = egptr() - eback();
    setp(m_buffer, m_buffer + m_capacity);
    pbump(old_write_offset);

    sputc(__c);
    setg(pbase(), pbase() + old_read_offset, pptr());

    return __c;
  }

  pos_type seekpos(pos_type __sp, ios_base::openmode __which) override {
    return seekoff(__sp, std::ios_base::beg, __which);
  }

  pos_type seekoff(off_type __off, ios_base::seekdir __way, ios_base::openmode __which) override {
    if (__which & std::ios_base::in) {
      auto base = eback();
      auto cur = gptr();
      auto max = egptr();
      auto req = calculate_target_offset(__off, __way, base, cur, max);
      if (req < base || req > max) {
        return pos_type(off_type(-1));
      }
      setg(base, req, max);
      return pos_type{req - base};
    }

    if (__which & std::ios_base::out) {
      auto base = pbase();
      auto cur = pptr();
      auto max = epptr();
      auto req = calculate_target_offset(__off, __way, base, cur, max);
      if (req < base || req > max) {
        return pos_type(off_type(-1));
      }
      setp(base, max);
      pbump(req - base);
      return pos_type{req - base};
    }

    assert(false && "unexpected openmode");
  }

protected:
  char* calculate_target_offset(off_type off, ios_base::seekdir seekdir, char* base, char* cur, char* max) {
    switch (seekdir) {
      case std::ios_base::beg: {
        return base + off;
      }
      case std::ios_base::end: {
        return max + off;
      }
      case std::ios_base::cur: {
        return cur + off;
      }
    }

    assert(false && "unexpected seekdir");
  }
};

int main() {
  try {
//    std::stringstream buf;
    MyStream buf(1);
    buf << "1111111111\n";
    buf << "2222222222\n";
    auto bkp = buf.tellp();
    buf.seekp(11);
    buf << "AAAAAAAAAA\n";
    buf.seekp(bkp);
    buf << "3333333333\n";
    buf << "4444444444\n";
    buf << "5555555555\n";

//    {
//      std::string line;
//      while (std::getline(buf, line)) {
//        std::cout << "read line: " << line << std::endl;
//      }
//    }
//    std::cout << "middle read done" << std::endl;

    auto bkp2 = buf.tellp();
    buf << "6666666666\n";
    buf << "7777777777\n";
    buf << "8888888888\n";
    buf << "9999999999\n";

    auto bkp3 = buf.tellp();
    buf.seekp(bkp2);
    buf << "XXXXXXXXXX\n";
    buf.seekp(bkp3);

    buf << "hehe" << std::endl;

    char tmpstr[] = "this is a long test sentence that needs to be copied into the buffer\n";
    buf.write(tmpstr, 69);
//    for (int i = 0; i < sizeof(tmpstr); i++) {
//      buf.sputc(tmpstr[i]);
//    }

    std::cout << "beginning read" << std::endl;

//    const size_t copybufsize = 32;
//    char copybuf[copybufsize] = {0};
//    size_t bytes_read = 0;
//    while ((bytes_read = buf.readsome(copybuf, copybufsize))) {
//      std::cout << "[";
//      std::cout.write(copybuf, bytes_read);
//      std::cout << "]";
//      std::cout << std::endl;
//    }

    {
      std::string line;
      while (std::getline(buf, line)) {
        std::cout << "read line: " << line << std::endl;
      }
    }
  } catch(std::runtime_error& error) {
    std::cout << "error: " << error.what() << std::endl;
  }
}
