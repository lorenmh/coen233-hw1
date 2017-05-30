#ifndef __TYPES__
#define __TYPES__

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

#define DELIMITER 0xffff
#define DELIMITER_STR "\xff\xff"
#define CLIENT_TABLE_SIZE 0xff
#define MAX_PACKET_SIZE 260
#define ACK_TIMER 3
#define MAX_CLIENT_RETRY 3

typedef struct demo_packet {
	int size;
	uint8_t buf[128];
} demo_packet;

typedef enum packet_t {
	DATA = 0xfff1,
	ACK = 0xfff2,
	REJECT = 0xfff3,
	ACC_PER = 0xfff8,
	NOT_PAID = 0xfff9,
	NOT_EXIST = 0xfffa,
	ACCESS_OK = 0xfffb
} packet_t;

typedef enum reject_t {
	OUT_OF_SEQ = 0xfff4,
	LEN_MISMATCH = 0xfff5,
	END_PACKET_MISSING = 0xfff6,
	DUP_PACKET = 0xfff7
} reject_t;

typedef enum return_t {
	SUCCESS = 0,
	ERR_OPEN_DELIMITER,
	ERR_CLOSE_DELIMITER,
	ERR_LEN_MISMATCH,
	ERR_INVALID_FMT,
	DB_AUTHORIZED,
	DB_NOT_FOUND,
	DB_NOT_AUTHORIZED
} return_t;

// packet is the generic packet type
// it has the size of the largest packet (so no segfaults occur)
typedef struct packet {
	uint8_t client_id;
	uint16_t type;
	uint8_t buf[MAX_PACKET_SIZE - 3]; // client_id and type are 3 bytes
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

//typedef struct client {
//	uint8_t segment_no;
//} client;
//
//typedef struct client_table {
//	client *ptrs[CLIENT_TABLE_SIZE];
//	client buf[CLIENT_TABLE_SIZE];
//} client_table;

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
