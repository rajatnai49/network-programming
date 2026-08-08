#include "rio.h"
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

ssize_t rio_readn(int fd, void* ubuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *buf = ubuf;

  while (nleft > 0) {
    nread = read(fd, buf, nleft);
    if(nread < 0) {
      if(errno == EINTR)
        nread = 0;
      else
        return -1;
    }
    else if(nread == 0)
      break;
    nleft -= nread;
    buf += nread;
  }

  return (n-nleft);
}

ssize_t rio_writen(int fd, void* ubuf, size_t n) {
  size_t nleft = n;
  ssize_t nwrite;
  char *buf = ubuf;

  while (nleft > 0) {
    nwrite = write(fd, buf, nleft);
    if(nwrite < 0) {
      if (errno == EINTR)
        nwrite = 0;
      else
        return -1;
    }
    else if(nwrite == 0)
      break;
    nleft -= nwrite;
    buf += nwrite;
  }

  return (n-nleft);
}

