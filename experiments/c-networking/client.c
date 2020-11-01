// based on the tutorials of https://www.youtube.com/channel/UCwd5VFu4KoJNjkWJZMFJGHQ

#include "common.h"

int main(int argc, char** argv) {
  int sockfd;
  int n;
  int sendbytes;
  sockaddr_in servaddr;
  char sendline[MAXLINE];
  char recvline[MAXLINE];

  // check correct usage
  if (argc != 2) {
    err_n_die("usage: %s <server address>", argv[0]);
  }

  // create socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err_n_die("Error while creating the socket!");
  }

  // clear and setup servaddr struct
  memset(&servaddr, 0, sizeof(sockaddr_in));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_port        = htons(SERVER_PORT); // server port

  // convert string ip to number format
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    err_n_die("inet_pton, error for %s", argv[1]);
  }

  // connect to the server
  if (connect(sockfd, (sockaddr*)&servaddr, sizeof(sockaddr)) < 0) {
    err_n_die("connect failed!");
  }

  // we're connected, prepare the message
  sprintf(sendline, "hello world this is the client speaking!!\n");
  sendbytes = strlen(sendline);

  // send the request
  if (write(sockfd, sendline, sendbytes) != sendbytes) {
    err_n_die("could not write all bytes to socket");
  }

  // read the servers response
  memset(recvline, 0, MAXLINE);
  while ((n = recv(sockfd, recvline, MAXLINE - 1, 0)) > 0) {
    printf("%s", recvline);
    fflush(stdout);
    memset(recvline, 0, MAXLINE);
  }

  if (n < 0) {
    err_n_die("could not read from socket");
  }

  exit(0);
  return 0;
}
