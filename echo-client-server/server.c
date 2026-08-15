#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 1024

int open_listenfd(char *port) {
  char *end;
  long port_num = strtol(port, &end, 10);

  if (*end != '\0') {
    fprintf(stderr, "%s is not a valid number\n", port);
    return 0;
  }

  int listenfd;
  struct sockaddr_in serveraddr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(port_num);
  inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr);

  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    close(listenfd);
    return -1;
  }

  if (listen(listenfd, 5) < 0) {
    fprintf(stderr, "error in the listen");
    close(listenfd);
    return -1;
  }

  return listenfd;
}

void echo(int connfd) {
  char buf[MAXLINE];
  ssize_t n;

  while ((n = read(connfd, buf, MAXLINE)) > 0) {
    printf("server received %zd bytes\n", n);
    write(connfd, buf, n);
  }
}

int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    if(connfd < 0) {
      close(listenfd);
      return 0;
    }
    printf("Connected");
    echo(connfd);
    close(connfd);
  }
  close(listenfd);
  exit(0);
}
