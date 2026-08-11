#include "rio.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

void rio_readinitb(rio_t *rp, int fd) {
  rp->rio_fd = fd;
  rp->rio_cnt = 0;
  rp->rio_bufptr = rp->rio_buf;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
  size_t cnt;

  // Refill buf with read
  while (rp->rio_cnt <= 0) {
    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));

    if (rp->rio_cnt < 0) {
      if (errno == EINTR)
        rp->rio_cnt = 0;
      else
        return -1;
    } else if (rp->rio_cnt == 0)
      return 0;
    else
      rp->rio_bufptr = rp->rio_buf;
  }

  // Copy bytes from internal buf to usrbuf
  cnt = n;
  if (rp->rio_cnt < n)
    cnt = rp->rio_cnt;

  memcpy(usrbuf, rp->rio_bufptr, cnt);
  rp->rio_bufptr += cnt;
  rp->rio_cnt -= cnt;
  return cnt;
}

ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *buf = usrbuf;

  while (nleft > 0) {
    nread = read(fd, buf, nleft);
    if (nread < 0) {
      if (errno == EINTR)
        nread = 0;
      else
        return -1;
    } else if (nread == 0)
      break;
    nleft -= nread;
    buf += nread;
  }

  return (n - nleft);
}

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *buf = usrbuf;

  while (nleft > 0) {
    nread = rio_read(rp, buf, nleft);
    if (nread < 0)
      return -1;
    else if (nread == 0)
      break;

    nleft -= nread;
    buf += nread;
  }

  return (n - nleft);
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
  int n, rc;
  char c, *buf = usrbuf;

  for (n = 1; n < maxlen; n++) {
    rc = rio_read(rp, &c, 1);
    if (rc == 1) {
      *buf++ = c;
      if (c == '\n') {
        n++;
        break;
      }
    } else if (rc == 0) {
      if (n == 1)
        return 0;
      else
        break;
    } else
      return -1;
  }

  *buf = 0;
  return n - 1;
}

ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nwrite;
  char *buf = usrbuf;

  while (nleft > 0) {
    nwrite = write(fd, buf, nleft);
    if (nwrite <= 0) {
      if (errno == EINTR)
        nwrite = 0;
      else
        return -1;
    }
    nleft -= nwrite;
    buf += nwrite;
  }

  return n;
}
