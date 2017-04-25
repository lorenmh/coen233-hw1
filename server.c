#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

// function prototypes
void resolve_packet_header(char*,packet*);

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Error: Missing required arg, needs <server-hostname> ");
    fprintf(stderr, "<server-port> <client-hostname> <client-port>\n");
    return 1;
  }

  char const *server_hostname = argv[1];
  char const *server_port = argv[2];
  char const *client_hostname = argv[3];
  char const *client_port = argv[4];

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

  int err = getaddrinfo(server_hostname, server_port, &hints, &results);

  if (err) {
    fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
  }

  struct sockaddr_in *addr = NULL;

  for (result = results; result != NULL; result = result->ai_next) {
    err = bind(socket_fd, result->ai_addr, result->ai_addrlen);

    if (err) {
      // fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
    } else {
      addr = (struct sockaddr_in *)result->ai_addr;
      printf("Bind successful, address: %s\n", inet_ntoa(addr->sin_addr));
      break;
    }
  }

  if (!addr) {
    fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
  }

  char buf[1024];
  memset(buf, 0, sizeof(buf));

  socklen_t len = sizeof(addr);

  int n = recvfrom(
    socket_fd,
    buf,
    1024,
    0,
    (struct sockaddr *)addr,
    &len
  );

  packet p;
  resolve_packet_header(buf, &p);

  printf(
    "RECEIVED: client_id: %d, type: %d\n",
    p.client_id,
    p.type
  );
}
