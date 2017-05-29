#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

// function prototypes
int parse_packet_buf(uint8_t*,int,packet*);
int ptos(packet*,return_t,char*);
int resolve_response_packet(packet*,return_t,packet*,uint8_t*,FILE*);
void hexp(uint8_t*,int);

//uint8_t demo1_packet_size = 11;
//uint8_t demo1[] = {
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x00, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x01, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x02, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x03, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x04, 0x02, 0x48, 0x69, 0xff, 0xff
//};
//
//uint8_t demo2_packet_size = 11;
//uint8_t demo2[] = {
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x00, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x02, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x01, 0x01, 0x48, 0x00, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x03, 0x02, 0x48, 0x69, 0xff, 0xff,
//	0xff, 0xff, 0x00, 0xff, 0xf1, 0x04, 0x02, 0x48, 0x69, 0xff, 0xff
//};

demo_packet demo1[] = {
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x00,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x01,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x02,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x03,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x04,0x02,0x48,0x69,0xff,0xff}}
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

	uint8_t client_table[CLIENT_TABLE_SIZE];
	memset(client_table, 0, sizeof(client_table));

	packet req_p;
	packet res_p;
	uint8_t buf[1024];

	//uint8_t *demo = demo1;
	//uint8_t demo_packet_size = demo1_packet_size;
	//uint8_t demo_len = sizeof(demo1) / demo_packet_size;

	demo_packet *demo = demo1;
	int demo_len = sizeof(demo1) / sizeof(demo1[0]);

	struct timeval ack_timer;
	ack_timer.tv_sec = ACK_TIMER;

	err = setsockopt(
			socket_fd,
			SOL_SOCKET,
			SO_RCVTIMEO,
			&ack_timer,
			sizeof(ack_timer)
	);

	if (err < 0) {
		fprintf(stderr, "Error setting socket option for timeout\n");
	}

	for(int i = 0; i < demo_len; i++) {
		demo_packet dp = demo[i];


		int retry_counter = 0;
		int received_response = 0;

		while(retry_counter < MAX_CLIENT_RETRY && !received_response) {
			sendto(
				socket_fd,
				dp.buf,
				dp.size,
				0,
				(struct sockaddr*) &server_addr,
				sizeof(server_addr)
			);

			char str[1024];

			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
			printf(
				"[SENT] address: %s, port: %d\n",
				inet_ntoa(server_addr.sin_addr),
				ntohs(server_addr.sin_port)
			);

			hexp(dp.buf, dp.size);

			err = parse_packet_buf(
					dp.buf,
					dp.size,
					&req_p
			);

			ptos(&req_p, err, str);
			printf("%s", str);

			memset(buf, 0, sizeof(buf));

			socklen_t len = sizeof(client_addr);
			struct sockaddr_in response_addr;

			int n = recvfrom(
				socket_fd,
				buf,
				1024,
				0,
				(struct sockaddr*)&response_addr,
				&len
			);

			if (errno == EAGAIN) {
				retry_counter += 1;

				if (retry_counter < MAX_CLIENT_RETRY) {
					printf(
							"Did not receive ACK after %d seconds, retrying...\n",
							ACK_TIMER
					);
				}

				continue;
			}
			
			received_response = 1;

			printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
			printf(
				"[RECEIVED] address: %s, port: %d\n",
				inet_ntoa(response_addr.sin_addr),
				ntohs(response_addr.sin_port)
			);

			hexp(buf, n);

			err = parse_packet_buf(buf, n, &res_p);

			ptos(&res_p, err, str);
			printf("%s", str);
		}

		if (!received_response) {
			printf("Server does not respond\n");
			break;
		}
	}
}
