#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 1024

int open_clientfd(char *port) {
  char *end;
  long port_num = strtol(port, &end, 10);

  if (*end != '\0') {
    fprintf(stderr, "%s is not a valid number\n", port);
    return 0;
  }

  int clientfd;
  struct sockaddr_in serveraddr;

  clientfd = socket(AF_INET, SOCK_STREAM, 0);

  if (clientfd < 0) {
    fprintf(stderr, "socket client id err");
    return -1;
  }

  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_port = htons(port_num);
  serveraddr.sin_family = AF_INET;
  if (inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr) != 1) {
    fprintf(stderr, "error in the p to n");
    close(clientfd);
    return -1;
  }

  if (connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) <
      0) {
    fprintf(stderr, "connect error");
    close(clientfd);
    return -1;
  }

  return clientfd;
}

int main(int argc, char **argv) {
  int clientfd;
  char buf[MAXLINE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    return 1;
  }

  clientfd = open_clientfd(argv[1]);

  if (clientfd < 0) {
    return 1;
  }

  while (fgets(buf, MAXLINE, stdin) != NULL) {
    write(clientfd, buf, strlen(buf));
    ssize_t n = read(clientfd, buf, MAXLINE);
    if(n <= 0) break;
    buf[n] = '\0';
    printf("%s\n", buf);
  }
  close(clientfd);
  return 0;
}
