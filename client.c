#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

// function prototypes
int parse_packet_buf(char*,packet*);
int ptos(packet*,parser_return_t,char*);
int resolve_response_packet(packet*,parser_return_t,packet*,uint8_t*,FILE*);

uint8_t correct[][11] = {
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x00, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x01, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x02, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x03, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x04, 0x02, 0x48, 0x69, 0xff, 0xff}
};

uint8_t incorrect[][11] = {
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x00, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x01, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x02, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x03, 0x02, 0x48, 0x69, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff, 0xf1, 0x04, 0x02, 0x48, 0x69, 0xff, 0xff}
};

int main(int argc, char *argv[]) {
	// remove buffering from stdout
	setbuf(stdout, NULL);

	if (argc < 3) {
		fprintf(stderr, "Error: Missing required arg, needs <server-hostname> ");
		fprintf(stderr, "<server-port> [packet1 packet2 ...]\n");
		return 1;
	}

	char const *server_hostname = argv[1];
	char const *server_port = argv[2];

	int const server_port_int = atoi(server_port);

	int num_packets = argc - 3;
	packet packets[num_packets];

	for (int i = 0; i < num_packets; i++) {
		parse_packet_buf(argv[i + 3], &packets[i]);
	}

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

	// port 0 means it will bind to any port
	int err = getaddrinfo(server_hostname, 0, &hints, &results);

	if (err) {
		fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
	}

	int bind_success = 0;

	struct sockaddr_in client_addr;

	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t addr_len = sizeof(client_addr);

	for (result = results; result != NULL; result = result->ai_next) {
		// attempt bind
		err = bind(socket_fd, result->ai_addr, result->ai_addrlen);

		if (err) {
			// fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
		} else {
			memcpy(&client_addr, result->ai_addr, sizeof(*(result->ai_addr)));

			getsockname(socket_fd, (struct sockaddr*)&client_addr, &addr_len);

			printf(
				"Bind successful, host: %s, port: %d\n",
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port)
			);
			bind_success = 1;
			break;
		}
	}

	struct sockaddr_in server_addr;
	struct hostent *he;

	server_addr.sin_family = AF_INET;
	he = gethostbyname(server_hostname);
	memcpy(&(server_addr.sin_addr.s_addr), he->h_addr, he->h_length);
	server_addr.sin_port = htons(server_port_int);

	printf(
		"Sending to server, host: %s, port: %d\n",
		inet_ntoa(server_addr.sin_addr),
		ntohs(server_addr.sin_port)
	);

	char udp_buf[1024];

	uint8_t client_table[CLIENT_TABLE_SIZE];
	memset(client_table, 0, sizeof(client_table));

	packet req_p;
	packet res_p;

	int len = sizeof(correct) / sizeof(correct[0]);

	for(int i = 0; i < len; i++) {
		sendto(
			socket_fd,
			correct[i],
			sizeof(correct[i]),
			0,
			(struct sockaddr*) &server_addr,
			sizeof(server_addr)
		);
	}
}
