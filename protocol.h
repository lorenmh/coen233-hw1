#ifndef __TYPES__
#define __TYPES__

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

#define DELIMITER 0xffff
#define DELIMITER_STR "\xff\xff"
#define CLIENT_TABLE_SIZE 0xff
#define MAX_PACKET_SIZE 264

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

typedef enum return_t {
	SUCCESS = 0,
	ERR_OPEN_DELIMITER,
	ERR_CLOSE_DELIMITER,
	ERR_LEN_MISMATCH,
	ERR_INVALID_FMT
} return_t;

// generic packet type
typedef struct packet {
	uint8_t buf[MAX_PACKET_SIZE];
} packet;

typedef struct data_packet {
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
	uint8_t len;
	uint8_t *payload;
} data_packet;

typedef struct ack_packet {
	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
} ack_packet;

typedef struct reject_packet {
	uint8_t client_id;
	uint16_t type;
	uint16_t reject_id;
	uint8_t segment_id;
} reject_packet;

typedef struct client {
  uint8_t segment_no;
} client;

typedef struct client_table {
  client *ptrs[CLIENT_TABLE_SIZE];
  client buf[CLIENT_TABLE_SIZE];
} client_table;

//typedef struct packet_list_node {
//	struct packet_list_node* next;
//	packet* p;
//} packet_list_node;
//
//// linked list to hold packets from a client
//typedef struct packet_list {
//	packet_list_node *head;
//	packet_list_node *tail;
//	packet_list_node *buf;
//	uint16_t size;
//} packet_list;
//
//typedef struct client {
//	packet_t *packet_list;
//} client;
//
//typedef struct client_table {
//	client *table[CLIENT_TABLE_SIZE];
//	size_t size;
//} client_tree;


#endif
