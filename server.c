#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

// function prototypes
int parse_packet_buf(uint8_t*,packet*);
int ptos(packet*,parser_return_t,char*);
int resolve_response_packet(packet*,parser_return_t,packet*,uint8_t*,FILE*);

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

			socklen_t len = sizeof(addr);
			getsockname(socket_fd, (struct sockaddr*)&addr, &len);

			printf(
				"Bind successful, host: %s, port: %d\n",
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

	uint8_t udp_buf[1024];

	uint8_t client_table[CLIENT_TABLE_SIZE];
	memset(client_table, 0, sizeof(client_table));

	socklen_t len = sizeof(client_addr);

	packet req_p;
	packet res_p;

	for(;;) {
		memset(udp_buf, 0, sizeof(udp_buf));

		int n = recvfrom(
			socket_fd,
			udp_buf,
			1024,
			0,//MSG_WAITALL,
			(struct sockaddr*)&client_addr,
			&len
		);

		printf("----------------------------------------\n");
		printf(
			"[RECEIVED] address: %s, port: %d\n",
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port)
		);

		printf("\n");
		for (int i = 0; i < n; i++) {
			printf("\\x%02x", udp_buf[i]);
		}
		printf("\n");

		err = parse_packet_buf(udp_buf, &req_p);

		resolve_response_packet(&req_p, err, &res_p, client_table, NULL);

		char req_str[1024];
		char res_str[1024];

		printf("dafuck?\n");
		ptos(&req_p, err, req_str);
		printf("%s", req_str);

		//ptos(&res_p, SUCCESS, res_str);
		//printf("%s", res_str);


	}
}
