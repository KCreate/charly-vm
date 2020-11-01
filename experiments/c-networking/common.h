#ifndef _COMMON_H_
#define _COMMON_H_

// include system headers
#include <sys/socket.h>       // basic socket definitions
#include <sys/epoll.h>        // async networking
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>           // variadic functions
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define SERVER_PORT 3000
#define MAXLINE 4096
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

void err_n_die(const char* fmt, ...) {
  int errno_save;
  va_list ap;

  // backup errno for later use
  errno_save = errno;

  // print out the fmt+args to standard out
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  fflush(stdout);

  // print out error message if errno was set
  if (errno_save != 0) {
    fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
    fprintf(stdout, "\n");
    fflush(stdout);
  }
  va_end(ap);

  exit(1);
}

#endif
