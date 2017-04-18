#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

const char *HOSTNAME = "localhost";
const char *PORT = "8000";

int main(int argc, char *argv[]) {
  printf("yo\n");

  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket_fd < 0) {
    fprintf(stderr, "Could not create socket, fd=%d\n", socket_fd);
  }

  struct addrinfo hints;
  struct addrinfo *result;

  // clear out hints
  memset(&hints, 0, sizeof(hints));

  // set hints fields for getaddrinfo
  hints.ai_protocol = 0;
  hints.ai_flags = AI_ADDRCONFIG;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  int err = getaddrinfo(HOSTNAME, PORT, &hints, &result);

  if (err) {
    fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
  }
}
