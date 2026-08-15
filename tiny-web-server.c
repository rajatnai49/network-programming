#include "./rio/rio.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
    printf("accept returned %d, errno=%d (%s)\n", connectfd, errno,
           strerror(errno));

    getnameinfo((struct sockaddr *)&client_addr, clientlen, hostname, MAXLINE,
                port, MAXLINE, 0);
    printf("Accepted connection from (%s %s)\n", hostname, port);

    doit(connectfd);
    close(connectfd);
  }
}

void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rp;
  rio_readinitb(&rp, fd);
  rio_readlineb(&rp, buf, MAXLINE);
  printf("Print request headers: %s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not Implemented",
                "This method is not allowed.");
    return;
  }

  discard_headers(&rp);

  is_static = parse_uri(uri, filename, cgiargs);
  printf("Filename %s", filename);

  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not Found",
                "Specified file does not exist.");
    return;
  }

  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Not Authorised",
                  "User not have related permissions");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
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
  while (strcmp(buf, "\r\n")) {
    rio_readlineb(rp, buf, MAXLINE);
    // printf("req header line: %s\n", buf);
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

int parse_uri(char *uri, char *filename, char *cgiargs) {
  if (!strstr(uri, "bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  }
  return 0;
}

void serve_static(int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, buf[MAXLINE], filetype[MAXLINE];

  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-type: %s\r\n", buf, filetype);
  sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, filesize);
  rio_writen(fd, buf, strlen(buf));
  printf("Resoponse headers\n");
  printf("%s", buf);

  srcfd = open(filename, O_RDONLY, 0);
  srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  close(srcfd);
  rio_writen(fd, srcp, filesize);
  munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype) {
  char extension[10];
  char *dot = strrchr(filename, '.');
  if (dot != NULL) {
    strcpy(extension, dot + 1);

    if (!strcmp(extension, "html"))
      strcpy(filetype, "text/html");
    else if (!strcmp(extension, "gif"))
      strcpy(filetype, "image/gif");
    else if (!strcmp(extension, "png"))
      strcpy(filetype, "image/png");
    else if (!strcmp(extension, "jpg"))
      strcpy(filetype, "image/jpeg");
  } else
    strcpy(filetype, "text/plain");
}
