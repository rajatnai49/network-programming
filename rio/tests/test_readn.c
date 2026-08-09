#include "../rio.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  int fds[2], rio_fds[2];
  pipe(fds);
  pipe(rio_fds);
  char *input = "abcdefghijklmn";
  unsigned long len = strlen(input);

  pid_t pid = fork();

  if (pid == 0) {
    size_t segment_size = 4;
    size_t i = 0;

    while (i < len) {
      size_t n = segment_size;

      if (i + n > len)
        n = len - i;

      write(fds[1], input + i, n);
      write(rio_fds[1], input + i, n);

      i += n;

      sleep(1);
    }

    close(fds[1]);
    close(rio_fds[1]);
  } else if (pid > 0) {
    char buf[len + 1], rio_buf[len + 1];
    buf[len] = '\0';
    rio_buf[len] = '\0';

    size_t bytes_read = read(fds[0], buf, len);
    size_t rio_bytes_read = rio_readn(rio_fds[0], rio_buf, len);

    printf("read: bytes:%ld, output:%s\n", bytes_read, buf);
    printf("rio_read: bytes:%ld, output:%s\n", rio_bytes_read, rio_buf);

    close(fds[0]);
    close(rio_fds[0]);
  } else {
    printf("There is some error while creating child process\n");
  }

  wait(NULL);
}
