#include <arpa/inet.h>

int main() {
  struct sockaddr_in sa;
  // IPv4
  struct sockaddr_in6 sa6; // IPv6
  inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr));
  inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr));
}
