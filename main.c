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
  struct addrinfo *results;
  struct addrinfo *result;

  // clear out hints
  memset(&hints, 0, sizeof(hints));

  // set hints fields for getaddrinfo
  hints.ai_protocol = 0;
  hints.ai_flags = AI_ADDRCONFIG;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  int err = getaddrinfo(HOSTNAME, PORT, &hints, &results);

  if (err) {
    fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
  }

    // char hostname[NI_MAXHOST];

    // err = getnameinfo(
    //   result->ai_addr,
    //   result->ai_addrlen,
    //   hostname,
    //   NI_MAXHOST,
    //   NULL,
    //   0,
    //   0
    // );

    // if (err != 0) {
    //     fprintf(stderr, "Error invoking getnameinfo: %s\n", strerror(errno));
    // }

    // printf(
    //   "Hostname: %s, Port: %d",
    //   hostname,
    //   ((struct sockaddr_in*)result)->sin_port
    // );

  struct addrinfo *socket_addrinfo;

  for (result = results; result != NULL; result = result->ai_next) {
    err = bind(socket_fd, result->ai_addr, result->ai_addrlen);

    if (err) {
      fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
    } else {
      printf("Connected\n");
      socket_addrinfo = result;
      break;
    }
  }

  char buf[1024];
  socklen_t len = sizeof(result->ai_addr);

  int n = recvfrom(
    socket_fd,
    buf,
    1024,
    0,
    socket_addrinfo->ai_addr,
    &len
  );

  printf("After listen\n");


}
