#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
int parse_packet_buf(uint8_t*, int, packet*);
int ptos(packet*, return_t, char*);
int resolve_response_packet(packet*, return_t, packet*, uint8_t*, FILE*);
void hexp(uint8_t*, int);

/******************************************************************************
 * DEMO PACKETS
 *****************************************************************************/
demo_packet demo[] = {
	// CORRECT PACKETS, client id: 0, segments: 0-4
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x00,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x01,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x02,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x03,0x02,0x48,0x69,0xff,0xff}},
	{.size=11, .buf={0xff,0xff,0x00,0xff,0xf1,0x04,0x02,0x48,0x69,0xff,0xff}},

	// INCORRECT PACKETS, client id: 1
	// first packet is correct, as per the assignment instructions
	{.size=11, .buf={0xff,0xff,0x01,0xff,0xf1,0x00,0x02,0x48,0x69,0xff,0xff}},
	// out of sequence (previous segment 0, this one is 2)
	{.size=11, .buf={0xff,0xff,0x01,0xff,0xf1,0x02,0x02,0x48,0x69,0xff,0xff}},
	// len mismatch (len is 1, though 2 bytes are in the payload field)
	{.size=11, .buf={0xff,0xff,0x01,0xff,0xf1,0x01,0x01,0x48,0x69,0xff,0xff}},
	// end of packet identifier is incorrectly 0xfff0
	{.size=11, .buf={0xff,0xff,0x01,0xff,0xf1,0x01,0x02,0x48,0x69,0xff,0xf0}},
	// duplicated packet (resent segment 0)
	{.size=11, .buf={0xff,0xff,0x01,0xff,0xf1,0x00,0x02,0x48,0x69,0xff,0xff}},

	// ACCESS REQUESTS
	// ALLOWED: client id: 2, sn: 408-554-6805, tech: 04
	{
		.size=14,
		.buf={
			0xff,0xff,0x02,0xff,0xf8,0x00,0x05,0x04,0xf3,0x84,0x7f,0x35,0xff,0xff
		}
	},

	// NOT PAID: client id: 3, sn: 408-554-6805, tech: 01
	{
		.size=14,
		.buf={
			0xff,0xff,0x03,0xff,0xf8,0x00,0x05,0x01,0xf3,0x84,0x7f,0x35,0xff,0xff
		}
	},

	// NOT PAID: client id: 4, sn: 408-554-8821, tech: 03
	{
		.size=14,
		.buf={
			0xff,0xff,0x04,0xff,0xf8,0x00,0x05,0x03,0xf3,0x84,0x87,0x15,0xff,0xff
		}
	},

	// ALLOWED: client id: 5, sn: 408-554-8821, tech: 02
	{
		.size=14,
		.buf={
			0xff,0xff,0x05,0xff,0xf8,0x00,0x05,0x02,0xf3,0x84,0x87,0x15,0xff,0xff
		}
	},

	// NOT EXISTS: client id: 6, sn: 408-123-4567, tech: 01
	{
		.size=14,
		.buf={
			0xff,0xff,0x06,0xff,0xf8,0x00,0x05,0x02,0xf3,0x42,0xb2,0x87,0xff,0xff
		}
	}
};

int demo_len = sizeof(demo) / sizeof(demo[0]);

/******************************************************************************
 * MAIN
 *****************************************************************************/
int main(int argc, char *argv[]) {
	// remove buffering from stdout
	setbuf(stdout, NULL);

	// make sure the program was invoked with server args
	if (argc != 3) {
		fprintf(stderr, "Error: Missing required arg, needs <server-hostname> ");
		fprintf(stderr, "<server-port>\n");
		return 1;
	}

	// server information from argv
	char const *server_hostname = argv[1];
	char const *server_port = argv[2];
	int const server_port_int = atoi(server_port);

	// used throughout the app to check the return code of function invocations
	int return_code;

	// the UDP datagram socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		fprintf(stderr, "Could not create socket, fd=%d\n", sock);
	}

	struct timeval ack_timer;
	ack_timer.tv_sec = ACK_TIMER;

	return_code = setsockopt(
			sock,
			SOL_SOCKET,
			SO_RCVTIMEO,
			&ack_timer,
			sizeof(ack_timer)
	);

	if (!return_code) {
		fprintf(stderr, "Could not set timeout option on socket\n");
	}

	// HOST ADDRESS RESOLUTION
	// variables used for host address resolution
	struct addrinfo hints;
	struct addrinfo *results;
	struct addrinfo *result;

	// clear hints segment
	memset(&hints, 0, sizeof(hints));

	// set hints fields for getaddrinfo
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	// port 0 means it will bind to any port (ephemeral port)
	return_code = getaddrinfo(server_hostname, 0, &hints, &results);

	if (return_code) {
		fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
	}

	// variables used for binding
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t addr_len = sizeof(client_addr);

	int bind_success = 0;

	// ATTEMPT BIND
	for (result = results; result != NULL; result = result->ai_next) {
		return_code = bind(sock, result->ai_addr, result->ai_addrlen);

		if (!return_code) {
			memcpy(&client_addr, result->ai_addr, sizeof(*(result->ai_addr)));

			getsockname(sock, (struct sockaddr*)&client_addr, &addr_len);

			printf(
				"Bind successful, host: %s, port: %d\n",
				inet_ntoa(client_addr.sin_addr),
				ntohs(client_addr.sin_port)
			);
			bind_success = 1;
			break;
		}
	}

	if (!bind_success) {
		fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
	}

	// RESOLVE SERVER HOST TO ADDRESS
	// variables to resolve server address
	struct sockaddr_in server_addr;
	struct hostent *he;

	// populate values from hostent into server_addr, is used to send datagrams
	// to the server
	server_addr.sin_family = AF_INET;
	he = gethostbyname(server_hostname);
	memcpy(&(server_addr.sin_addr.s_addr), he->h_addr, he->h_length);
	server_addr.sin_port = htons(server_port_int);

	// initialize request packet and response packet onto stack
	packet req_p;
	packet res_p;

	// generic stack allocated buffer
	uint8_t buf[1024];
	// char buffer used to print information
	char str[1024];

	for(int i = 0; i < demo_len; i++) {
		// the packet that will be sent
		demo_packet dp = demo[i];

		// used to reinitiate request packets if necessary (timeout)
		int retry_counter = 0;
		int received_response = 0;

		// while loop to reinitiate request packets
		while(retry_counter < MAX_CLIENT_RETRY && !received_response) {
			// send UDP packet, blocking
			sendto(
				sock,
				dp.buf,
				dp.size,
				0,
				(struct sockaddr*) &server_addr,
				sizeof(server_addr)
			);

			// print information for packet sent
			printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
			printf(
				"[SENT] address: %s, port: %d\n",
				inet_ntoa(server_addr.sin_addr),
				ntohs(server_addr.sin_port)
			);

			// print packet as a hex string
			hexp(dp.buf, dp.size);

			// parse sent packet (for logging);
			return_code = parse_packet_buf(dp.buf, dp.size, &req_p);

			// populate str with text information for packet
			ptos(&req_p, return_code, str);

			// print text information for packet
			printf("%s", str);

			socklen_t len = sizeof(client_addr);
			struct sockaddr_in response_addr;

			int n = recvfrom(
				sock,
				buf,
				1024,
				0,
				(struct sockaddr*)&response_addr,
				&len
			);

			if (errno == EAGAIN || n <= 0) {
				retry_counter += 1;

				if (retry_counter <= MAX_CLIENT_RETRY) {
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

			return_code = parse_packet_buf(buf, n, &res_p);

			ptos(&res_p, return_code, str);
			printf("%s", str);
		}

		if (!received_response) {
			printf("Server does not respond\n");
			break;
		}
	}
}
