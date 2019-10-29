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

#include <queue>
#include <vector>
#include <unordered_map>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "value.h"

#pragma once

namespace Charly {

// TODO: CamelCase
enum AsyncTaskType : uint8_t {

  // Async File System Operations
  fs_access,
  fs_append_file,
  fs_chmod,
  fs_chown,
  fs_close,
  fs_copy_file,
  fs_exists,
  fs_fchmod,
  fs_fchown,
  fs_fdatasync,
  fs_fstat,
  fs_fsync,
  fs_ftruncate,
  fs_futimes,
  fs_lchmod,
  fs_lchown,
  fs_link,
  fs_lstat,
  fs_mkdir,
  fs_mkdtemp,
  fs_open,
  fs_opendir,
  fs_open_sync,
  fs_read,
  fs_readdir,
  fs_readfile,
  fs_readlink,
  fs_realpath,
  fs_rename,
  fs_rmdir,
  fs_stat,
  fs_symlink,
  fs_truncate,
  fs_unlink,
  fs_unwatch_file,
  fs_utimes,
  fs_watch,
  fs_watch_file,
  fs_write,
  fs_write_file,
  fs_writev,

  // File Descriptor Events
  fd_ondata,
  fd_onclose,
  fd_onend,
  fd_onerror,
  fd_onreadable,

  // Readline Events
  rl_onclose,
  rl_online,
  rl_onpause,
  rl_onresume,
  rl_sigcont,
  rl_sigint,
  rl_sigstp,
  rl_clearscreendown,
  rl_cursorto,
  rl_movecursor,

  Count
};

struct AsyncTask {
  AsyncTaskType type;
  std::vector<VALUE> arguments;
  VALUE cb;
};

struct AsyncTaskResult {
  VALUE cb;
  VALUE result;
};

}  // namespace Charly
