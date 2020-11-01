// based on the tutorials of https://www.youtube.com/channel/UCwd5VFu4KoJNjkWJZMFJGHQ

#include "common.h"

int main(int32_t argc, char** argv) {
  int32_t listenfd, connfd, n;
  sockaddr_in servaddr;
  char sendline[MAXLINE];
  char recvline[MAXLINE];

  // create socket
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err_n_die("could not create socket");
  }

  int always_true = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &always_true, sizeof(int)) < 0) {
    err_n_die("could not set socket options");
  }

  // configure allowed IPs and port
  memset(&servaddr, 0, sizeof(sockaddr_in));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(SERVER_PORT);

  if ((bind(listenfd, (sockaddr*)&servaddr, sizeof(sockaddr))) < 0) {
    err_n_die("could not bind");
  }

  if ((listen(listenfd, 1)) < 0) {
    err_n_die("could not listen");
  }

  uint64_t request_id = 0;
  for (;;) {
    sockaddr_in addr;
    socklen_t addr_len;

    // accept new connections using accept
    printf("waiting for connection #%lu on port %d\n", request_id++, SERVER_PORT);
    fflush(stdout);
    connfd = accept(listenfd, (sockaddr*)NULL, NULL);

    // clear recv buffer
    memset(recvline, 0, MAXLINE);

    // read client message
    while ((n = read(connfd, recvline, MAXLINE - 1)) > 0) {
      fprintf(stdout, "%s", recvline);

      // hacky way to detect end of the message
      if (recvline[n - 1] == '\n') {
        break;
      }

      memset(recvline, 0, MAXLINE);
    }
    if (n < 0) {
      err_n_die("error while reading request");
    }

    // send the response
    memset(sendline, 0, MAXLINE);
    snprintf(sendline, MAXLINE - 1, "HTTP/1.0 200 OK\r\n\r\nHello world!\n");

    write(connfd, sendline, strlen(sendline));
    close(connfd);

    fprintf(stdout, "\n");
  }

  return 0;
}
