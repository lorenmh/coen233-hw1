#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "protocol.h"

#define DB_FNAME "Verification_Database.txt"

/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
int parse_packet_buf(uint8_t*, int, packet*);
int ptos(packet*, return_t, char*);
int resolve_response_packet(packet*, return_t, packet*, uint8_t*, FILE*);
int ptob(packet*, uint8_t*);
void hexp(uint8_t*, int);
int query(FILE*, uint8_t*);

/******************************************************************************
 * MAIN
 *****************************************************************************/
int main(int argc, char *argv[]) {
	// remove buffering from stdout
	setbuf(stdout, NULL);

	if (argc != 3) {
		fprintf(stderr, "Error: Missing required arg, needs <server-hostname> ");
		fprintf(stderr, "<server-port>\n");
		return 1;
	}

	// server information from argv
	char const *server_hostname = argv[1];
	char const *server_port = argv[2];

	// used throughout the app to check the return code of function invocations
	int return_code;

	// the UDP datagram socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		fprintf(stderr, "Could not create socket, fd=%d\n", sock);
		return 1;
	}

	// initialize client table which contains the next expected segment id for a
	// given client index
	uint8_t client_table[CLIENT_TABLE_SIZE];
	memset(client_table, 0, sizeof(client_table));

	// the file DB used for verifying authentication
	FILE *db = fopen(DB_FNAME, "r");
	if (db == NULL) {
		fprintf(stderr, "Could not open file '%s'\n", DB_FNAME);
		return 1;
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

	return_code = getaddrinfo(server_hostname, server_port, &hints, &results);

	if (return_code) {
		fprintf(stderr, "Error invoking getaddrinfo: %s\n", strerror(errno));
		return 1;
	}

	// variables used for binding
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	socklen_t addr_len = sizeof(server_addr);

	int bind_success = 0;

	// ATTEMPT BIND
	for (result = results; result != NULL; result = result->ai_next) {
		return_code = bind(sock, result->ai_addr, result->ai_addrlen);

		if (!return_code) {
			memcpy(&server_addr, result->ai_addr, sizeof(*(result->ai_addr)));

			getsockname(sock, (struct sockaddr*)&server_addr, &addr_len);

			printf(
				"Bind successful, host: %s, port: %d\n",
				inet_ntoa(server_addr.sin_addr),
				ntohs(server_addr.sin_port)
			);
			bind_success = 1;
			break;
		}
	}

	if (!bind_success) {
		fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
		return 1;
	}

	// initialize client_addr, is used to send response datagrams
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t len = sizeof(client_addr);

	// initialize request packet and response packet onto stack
	packet req_p;
	packet res_p;

	// generic stack allocated buffer
	uint8_t buf[1024];
	// char buffer used to print information
	char str[1024];

	for(;;) {
		// receive UDP packets, blocking
		int n = recvfrom(
			sock,
			buf,
			sizeof(buf),
			0,//MSG_WAITALL,
			(struct sockaddr*)&client_addr,
			&len
		);

		// print information for packet received
		printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
		printf(
			"[RECEIVED] address: %s, port: %d\n",
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port)
		);

		// print packet as a hex string
		hexp(buf, n);

		// parse buffer into packet
		return_code = parse_packet_buf(buf, n, &req_p);

		// populate str with text information for received packet
		ptos(&req_p, return_code, str);

		// print text information for packet
		printf("%s", str);

		// generate a packet response
		resolve_response_packet(&req_p, return_code, &res_p, client_table, db);

		// convert packet to byte buffer
		n = ptob(&res_p, buf);

		// write byte buffer to socket
		sendto(
			sock,
			buf,
			n,
			0,
			(struct sockaddr*) &client_addr,
			sizeof(client_addr)
		);

		// print information for packet sent
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf(
			"[SENT] address: %s, port: %d\n",
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port)
		);

		// print packet as a hex string
		hexp(buf, n);

		// populate str with text information for sent packet
		ptos(&res_p, SUCCESS, str);

		// print text information for packet
		printf("%s", str);
	}

	return 0;
}
