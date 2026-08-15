#include "./rio/rio.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXLINE 8192

int open_listenfd(char *port);
void doit(int fd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void discard_headers(rio_t *rp);

int main(int argc, char **argv) {
  int listenfd, connectfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage client_addr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(client_addr);
    connectfd = accept(listenfd, (struct sockaddr *)&client_addr, &clientlen);

    getnameinfo((struct sockaddr *)&client_addr, clientlen, hostname, MAXLINE,
                port, MAXLINE, 0);
    printf("Accepted connection from (%s %s)\n", hostname, port);

    doit(connectfd);
    close(connectfd);
  }
}

void doit(int fd) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rp;
  rio_readinitb(&rp, fd);
  rio_readlineb(&rp, buf, MAXLINE);
  printf("Print request headers: %s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "200", "Not Implemented",
                "This method is not allowed");
    return;
  }

  discard_headers(&rp);
}

int open_listenfd(char *port) {
  char *end;
  long port_number = strtol(port, &end, 10);

  if (*end != '\0') {
    perror(port);
    perror("port is not valid number");
  }

  int listenfd;
  struct sockaddr_in server_addr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_number);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
    perror("Error in the bind");
    close(listenfd);
    return -1;
  }

  if (listen(listenfd, 5) < 0) {
    perror("Error in the listen");
    close(listenfd);
  }

  return listenfd;
}

void discard_headers(rio_t *rp) {
  char buf[MAXLINE];

  rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "/r/n")) {
    rio_readlineb(rp, buf, MAXLINE);
    printf("req header line: %s\n", buf);
  }

  return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>My Friend</title><html>");
  sprintf(body, "%s<body>\r\n", body);
  sprintf(body, "%s<b>%s</b>\r\n", body, shortmsg);
  sprintf(body, "%s<br><i>%s</i>\r\n", body, longmsg);
  sprintf(body, "%s</body>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  rio_writen(fd, buf, strlen(buf));
  rio_writen(fd, body, strlen(body));
}
