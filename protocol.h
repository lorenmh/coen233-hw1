#ifndef __TYPES__
#define __TYPES__

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

#define DELIMETER 0xFFFF
#define CLIENT_TABLE_SIZE 256
#define PACKET_LIST_INITIAL_SIZE 16
#define PACKET_LIST_FACTOR 2

typedef enum packet_t {
	DATA = 0xFFF1,
	ACK = 0xFFF2,
	REJECT = 0xFFF3
} packet_t;


typedef enum reject_t {
	OUT_OF_SEQ = 0xFFF4,
	LEN_MISMATCH = 0xFFF5,
	END_PACKET_MISSING = 0xFFF6,
	DUP_PACKET = 0xFFF7
} reject_t;

typedef enum error_t {
	ERR_OPEN_DELIMETER = 1,
	ERR_CLOSE_DELIMETER,
	ERR_INVALID_TYPE
} error_t;

typedef struct packet {
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
	uint8_t len;
	uint8_t *payload;
	uint8_t reject_id;
} packet;

typedef struct packet_list_node {
	struct packet_list_node* next;
	packet* p;
} packet_list_node;

// linked list to hold packets from a client
typedef struct packet_list {
	packet_list_node *head;
	packet_list_node *tail;
	packet_list_node *buf;
	uint16_t size;
} packet_list;

typedef struct client {
	packet_t *packet_list;
} client;

typedef struct client_table {
	client *table[CLIENT_TABLE_SIZE];
	size_t size;
} client_tree;


#endif
