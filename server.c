#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

// function prototypes
int resolve_packet(char*,void*,packet_t*);
int ptos(void*,packet_t,return_t,char*);

int main(int argc, char *argv[]) {
  // remove buffering from stdout
  setbuf(stdout, NULL);

  if (argc != 3) {
    fprintf(stderr, "Error: Missing required arg, needs <server-hostname> ");
    fprintf(stderr, "<server-port> <client-hostname> <client-port>\n");
    return 1;
  }

  char const *server_hostname = argv[1];
  char const *server_port = argv[2];

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

  int bind_success = 0;
  struct sockaddr_in addr;
  struct sockaddr_in client_addr;
  memset(&addr, 0, sizeof(addr));
  memset(&client_addr, 0, sizeof(client_addr));

  for (result = results; result != NULL; result = result->ai_next) {
    err = bind(socket_fd, result->ai_addr, result->ai_addrlen);

    if (err) {
      // fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
    } else {
      memcpy(&addr, result->ai_addr, sizeof(*(result->ai_addr)));
      printf(
        "Bind successful, address: %s, port: %d\n",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port)
      );
      bind_success = 1;
      break;
    }
  }

  if (!bind_success) {
    fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
  }

  char buf[1024];

  client client_table[0xff];
  memset(client_table, 0, sizeof(client_table));

  client c;

  socklen_t len = sizeof(client_addr);

  for(;;) {
    memset(buf, 0, sizeof(buf));

    int n = recvfrom(
      socket_fd,
      buf,
      1024,
      0,//MSG_WAITALL,
      (struct sockaddr*)&client_addr,
      &len
    );

    packet p;
		packet_t pt;
    err = resolve_packet(buf, &p, &pt);

    printf("----------------------------------------\n");
    printf(
      "[RECEIVED] address: %s, port: %d\n",
      inet_ntoa(client_addr.sin_addr),
      ntohs(client_addr.sin_port)
    );

		char str[1024];
		ptos(&p, pt, err, str);

		printf("%s", str);

    //printf("\terr: %d\n", err);
    //printf("\tclient_id: %d\n", p.client_id);
    //printf("\ttype: %d\n", p.type);
    //printf("\tlen: %d\n", p.len);
    //printf("\tpayload: '%.*s'\n", p.len, p.payload);
  }
}
