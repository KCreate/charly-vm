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

#include <sstream>
#include <chrono>
#include <time.h>

#include "time.h"

#include "vm.h"

namespace Charly {
namespace Internals {
namespace Time {

using TimepointSystem = std::chrono::time_point<std::chrono::system_clock>;
using TimepointSteady = std::chrono::time_point<std::chrono::steady_clock>;

VALUE system_clock_now(VM& vm) {
  (void)vm;
  TimepointSystem now = std::chrono::system_clock::now();
  return charly_create_double(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

VALUE steady_clock_now(VM& vm) {
  (void)vm;
  TimepointSteady now = std::chrono::steady_clock::now();
  return charly_create_double(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

VALUE highres_now(VM& vm) {
  (void)vm;
  std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - vm.starttime);
  return charly_create_double(ns.count());
}

VALUE to_local(VM& vm, VALUE ts) {
  CHECK(number, ts);

  int64_t ms = charly_number_to_int64(ts);
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
  std::time_t time_obj = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *(std::localtime(&time_obj));

  char buf[26] = {0};
  // std::strftime(buf, sizeof(buf), "Www Mmm dd hh:mm:ss yyyy", &tm);
  std::strftime(buf, sizeof(buf), "%a %d. %b %Y %H:%M:%S", &tm);

  return vm.create_string(buf, sizeof(buf));
}

VALUE to_utc(VM& vm, VALUE ts) {
  CHECK(number, ts);

  int64_t ms = charly_number_to_int64(ts);
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
  std::time_t time_obj = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *(std::gmtime(&time_obj));

  char buf[26] = {0};
  // std::strftime(buf, sizeof(buf), "Www Mmm dd hh:mm:ss yyyy", &tm);
  std::strftime(buf, sizeof(buf), "%a %d. %b %Y %H:%M:%S", &tm);

  return vm.create_string(buf, sizeof(buf));
}

VALUE fmt(VM& vm, VALUE ts, VALUE fmt) {
  CHECK(number, ts);
  CHECK(string, fmt);

  // Get the timestamp
  int64_t ms = charly_number_to_int64(ts);
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
  std::time_t time_obj = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *(std::localtime(&time_obj));

  // Copy the format string into a local buffer
  char* fmt_data = charly_string_data(fmt);
  uint32_t fmt_length = charly_string_length(fmt);

  // If our format is bigger than our pre-allocated local buffer we have
  // to return with an error message
  //
  // We subtract 1 to allow space for the null terminator
  char format_buf[256] = {0};
  if (fmt_length > sizeof(format_buf) - 1) {
    return kNull;
  }
  std::memcpy(format_buf, fmt_data, fmt_length);

  char result_buf[26] = {0};
  std::strftime(result_buf, sizeof(result_buf), format_buf, &tm);

  return vm.create_string(result_buf, strlen(result_buf));
}

VALUE fmtutc(VM& vm, VALUE ts, VALUE fmt) {
  CHECK(number, ts);
  CHECK(string, fmt);

  // Get the timestamp
  int64_t ms = charly_number_to_int64(ts);
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
  std::time_t time_obj = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *(std::gmtime(&time_obj));

  // Copy the format string into a local buffer
  char* fmt_data = charly_string_data(fmt);
  uint32_t fmt_length = charly_string_length(fmt);

  // If our format is bigger than our pre-allocated local buffer we have
  // to return with an error message
  //
  // We subtract 1 to allow space for the null terminator
  char format_buf[256] = {0};
  if (fmt_length > sizeof(format_buf) - 1) {
    return kNull;
  }
  std::memcpy(format_buf, fmt_data, fmt_length);

  char result_buf[26] = {0};
  std::strftime(result_buf, sizeof(result_buf), format_buf, &tm);

  return vm.create_string(result_buf, strlen(result_buf));
}

VALUE parse(VM& vm, VALUE src, VALUE fmt) {
  CHECK(string, src);
  CHECK(string, fmt);

  // Copy source and format strings
  std::string source_string(charly_string_data(src), charly_string_length(src));
  std::string format_string(charly_string_data(fmt), charly_string_length(fmt));

  std::tm tm = {};
  std::istringstream(source_string) >> std::get_time(&tm, format_string.c_str());
  auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  return charly_create_double(std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count());
}

}  // namespace Time
}  // namespace Internals
}  // namespace Charly
