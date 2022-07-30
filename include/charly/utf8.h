/*
 * This file is part of the Charly Virtual Machine (https://github.com/KCreate/charly-vm)
 *
 * MIT License
 *
 * Copyright (c) 2017 - 2022 Leonard Schütz
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

#include <string>

#pragma once

namespace charly::utf8 {

namespace internal {
  const uint32_t LEAD_SURROGATE_MIN = 0x0000d800;
  const uint32_t TRAIL_SURROGATE_MAX = 0x0000dfff;
  const uint32_t CODE_POINT_MAX = 0x0010ffff;

  inline uint32_t sequence_length(const char* buffer) {
    auto lead_byte = static_cast<uint8_t>(*buffer);
    if (lead_byte < 0x80) {
      return 1;
    } else if ((lead_byte >> 5) == 0x6) {
      return 2;
    } else if ((lead_byte >> 4) == 0xe) {
      return 3;
    } else if ((lead_byte >> 3) == 0x1e) {
      return 4;
    } else {
      return 0;
    }
  }

  inline bool is_surrogate(uint16_t cp) {
    return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
  }

  inline bool is_trail(uint16_t cp) {
    return ((cp & 0xff) >> 6) == 0x2;
  }

  inline bool is_valid_code_point(uint32_t cp) {
    return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
  }

  inline bool is_overlong_sequence(uint32_t cp, uint32_t length) {
    if (cp < 0x80) {
      if (length != 1)
        return true;
    } else if (cp < 0x800) {
      if (length != 2)
        return true;
    } else if (cp < 0x10000) {
      if (length != 3)
        return true;
    }

    return false;
  }

  inline bool increase_safely(const char*& buffer, const char* buffer_end) {
    if (++buffer == buffer_end) {
      return false;
    }

    if (!is_trail(*buffer)) {
      return false;
    }

    return true;
  }

  inline bool decode_1(const char*& buffer, const char* buffer_end, uint32_t& cp_out) {
    if (buffer == buffer_end) {
      return false;
    }

    cp_out = static_cast<uint8_t>(*buffer);

    return true;
  }

  inline bool decode_2(const char*& buffer, const char* buffer_end, uint32_t& cp_out) {
    if (buffer == buffer_end) {
      return false;
    }

    cp_out = static_cast<uint8_t>(*buffer);

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out = ((cp_out << 6) & 0x7ff) + ((*buffer) & 0x3f);

    return true;
  }

  inline bool decode_3(const char*& buffer, const char* buffer_end, uint32_t& cp_out) {
    if (buffer == buffer_end) {
      return false;
    }

    cp_out = static_cast<uint8_t>(*buffer);

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out = ((cp_out << 12) & 0xffff) + ((*buffer << 6) & 0xfff);

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out += (*buffer) & 0x3f;

    return true;
  }

  inline bool decode_4(const char*& buffer, const char* buffer_end, uint32_t& cp_out) {
    if (buffer == buffer_end) {
      return false;
    }

    cp_out = static_cast<uint8_t>(*buffer);

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out = ((cp_out << 18) & 0x1fffff) + ((*buffer << 12) & 0x3ffff);

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out += (*buffer << 6) & 0xfff;

    if (!increase_safely(buffer, buffer_end)) {
      return false;
    }

    cp_out += *buffer & 0x3f;

    return true;
  }
}  // namespace internal

inline uint32_t sequence_length(uint32_t cp) {
  if (cp < 0x80)
    return 1;
  if (cp < 0x800)
    return 2;
  if (cp < 0x10000)
    return 3;
  return 4;
}

[[nodiscard]] inline bool next(const char*& buffer, const char* buffer_end, uint32_t& cp_out) {
  if (buffer >= buffer_end) {
    return false;
  }

  const char* original_buffer = buffer;
  uint32_t cp = 0;
  uint32_t length = internal::sequence_length(buffer);

  bool success;
  switch (length) {
    case 0: return false;
    case 1: success = internal::decode_1(buffer, buffer_end, cp); break;
    case 2: success = internal::decode_2(buffer, buffer_end, cp); break;
    case 3: success = internal::decode_3(buffer, buffer_end, cp); break;
    case 4: success = internal::decode_4(buffer, buffer_end, cp); break;
    default: return false;
  }

  if (success) {
    if (internal::is_valid_code_point(cp)) {
      if (!internal::is_overlong_sequence(cp, length)) {
        cp_out = cp;
        buffer++;
        return true;
      }
    }
  }

  buffer = original_buffer;
  return false;
}

[[nodiscard]] inline bool next(const char*& buffer, const char* buffer_end) {
  uint32_t cp;
  return next(buffer, buffer_end, cp);
}

[[nodiscard]] inline bool peek_next(const char* buffer, const char* buffer_end, uint32_t& cp_out) {
  return next(buffer, buffer_end, cp_out);
}

[[nodiscard]] inline char* append(uint32_t cp, char* buffer) {
  if (!internal::is_valid_code_point(cp)) {
    return nullptr;
  }

  if (cp < 0x80) {
    *(buffer++) = static_cast<char>(cp);
  } else if (cp < 0x800) {
    *(buffer++) = static_cast<char>((cp >> 6) | 0xc0);
    *(buffer++) = static_cast<char>((cp & 0x3f) | 0x80);
  } else if (cp < 0x10000) {
    *(buffer++) = static_cast<char>((cp >> 12) | 0xe0);
    *(buffer++) = static_cast<char>(((cp >> 6) & 0x3f) | 0x80);
    *(buffer++) = static_cast<char>((cp & 0x3f) | 0x80);
  } else {
    *(buffer++) = static_cast<char>((cp >> 18) | 0xf0);
    *(buffer++) = static_cast<char>(((cp >> 12) & 0x3f) | 0x80);
    *(buffer++) = static_cast<char>(((cp >> 6) & 0x3f) | 0x80);
    *(buffer++) = static_cast<char>((cp & 0x3f) | 0x80);
  }

  return buffer;
}

inline bool is_valid_codepoint(uint32_t cp) {
  return internal::is_valid_code_point(cp);
}

inline std::string codepoint_to_string(uint32_t cp) {
  char buf[4];
  char* end = append(cp, buf);
  return { buf, (size_t)(end - buf) };
}

}  // namespace charly::utf8