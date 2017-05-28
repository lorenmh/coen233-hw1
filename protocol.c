#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include "protocol.h"

/*============================================================================
 * protocol.c:
 * -----------
 * Contains utility functions for implementing the protcol.
=============================================================================*/

/*============================================================================
 * void ptos(...):
 * ---------------
 * Populates a char* with text information for a given packet
 * 
 * args:
 * -----
 * void *p: a pointer to a packet
 * packet_t pt: a packet type value
 * parser_return_t rt: a parser return type value
 * char *s: An allocated string in which the text data will be populated
=============================================================================*/
void ptos(packet *p, parser_return_t rt, char *s) {
	char *resolution_str = "";
	if (rt == SUCCESS) {
		resolution_str = "PARSER SUCCESS";
	} else if (rt == ERR_OPEN_DELIMITER) {
		resolution_str = "PARSER ERROR: ERR_OPEN_DELIMITER";
	} else if (rt == ERR_CLOSE_DELIMITER) {
		resolution_str = "PARSER ERROR: ERR_CLOSE_DELIMITER";
	} else if (rt == ERR_LEN_MISMATCH) {
		resolution_str = "PARSER ERROR: ERR_LEN_MISMATCH";
	} else if (rt == ERR_INVALID_FMT) {
		resolution_str = "PARSER ERROR: ERR_INVALID_FMT";
	} else {
		resolution_str = "PARSER INFORMATION NOT PROVIDED";
	}

	packet_t pt = p->type;

	if (pt == DATA || pt == ACC_PER || pt == NOT_PAID ||
			pt == NOT_EXIST || pt == ACCESS_OK) {
		data_packet *dp = (data_packet*) p;

		char *type_s = "DATA";
		if (pt == ACC_PER) {
			type_s = "ACC_PER";
		} else if (pt == NOT_PAID) {
			type_s = "NOT_PAID";
		} else if (pt == NOT_EXIST) {
			type_s = "NOT_EXIST";
		} else if (pt == ACCESS_OK) {
			type_s = "ACCESS_OK";
		}

		sprintf(
				s,
				"%s\n"
				"Client ID: 0x%02x\n"
				"Type: 0x%04x [%s]\n"
				"Segment No: 0x%02x\n"
				"Length: 0x%02x\n"
				"Payload: '%s'\n",
				resolution_str,
				dp->client_id,
				dp->type,
				type_s,
				dp->segment_id,
				dp->len,
				dp->payload
		);
		return;
	}

	if (pt == ACK) {
		ack_packet *ap = (ack_packet*) p;
		sprintf(
				s,
				"%s\n"
				"Client ID: 0x%02x\n"
				"Type: 0x%04x [ACK]\n"
				"Received Segment No: 0x%02x\n",
				resolution_str,
				ap->client_id,
				ap->type,
				ap->segment_id
		);
		return;
	}

	if (pt == REJECT) {
		reject_packet *rp = (reject_packet*) p;
		uint16_t reject_id = rp->reject_id;

		char *reject_str = "";

		if (reject_id == OUT_OF_SEQ) {
			reject_str = "OUT_OF_SEQ";
		} else if (reject_id == LEN_MISMATCH) {
			reject_str = "LEN_MISMATCH";
		} else if (reject_id == END_PACKET_MISSING) {
			reject_str = "END_PACKET_MISSING";
		} else if (reject_id == DUP_PACKET) {
			reject_str = "DUP_PACKET";
		} else {
			reject_str = "INVALID_REJECT_ID";
		}

		sprintf(
				s,
				"%s\n"
				"Client ID: 0x%02x\n"
				"Type: 0x%04x [REJECT]\n"
				"Reject Sub Code: 0x%04x [%s]\n"
				"Received Segment No: 0x%02x\n",
				resolution_str,
				rp->client_id,
				rp->type,
				rp->reject_id,
				reject_str,
				rp->segment_id
		);
	}
}

/*============================================================================
 * void response_packet(...):
 * --------------------------
 * Populates a response packet with data. When given a request packet, if the
 * packet is valid then response_packet will perform a lookup in the
 * client_table to make sure the segment number is correct. If the request is
 * an access request of some kind, response_packet will perform a lookup in the
 * verification_db and will populate a response packet given this information.
 * 
 * args:
 * -----
 * packet const *cp: a pointer to the client packet
 * packet_t ct: the client packet type
 * parser_return_t prt: the parser return type for the client packet
 * packet* const rp: a pointer to the response packet which will be populated
 *   with data
 * packet_t* const rt: a pointer to the response packet type which will be
 *   populated with the value of the response packet type
 * uint8_t* const client_table: a table of expected segment numbers for clients
 * FILE* verification_db: a pointer to the Verification_Database.txt file
=============================================================================*/
void response_packet(
		packet const *req_p,
		parser_return_t const req_prt,
		packet* const res_p,
		uint8_t* const client_table,
		FILE* verification_db) {

	// only server can send ACK/REJECT
	if (req_p->type == ACK || req_p->type == REJECT || req_prt != SUCCESS) {
		reject_packet *res_rp = (reject_packet*) res_p;
		data_packet *req_dp = (data_packet*) req_p;

		if (req_p->type == DATA && req_prt == ERR_CLOSE_DELIMITER) {
			res_rp->client_id = req_dp->client_id;
			res_rp->type = REJECT;
			res_rp->reject_id = END_PACKET_MISSING;
			res_rp->segment_id = req_dp->segment_id;
			return;
		}

		if (req_p->type == DATA && req_prt == ERR_LEN_MISMATCH) {
		}

		// generic reject response;
	}
}

parser_return_t parse_packet_buf(
		char const *buf,
		void* const p) {

	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
	uint8_t len = 0;
	uint8_t *payload = 0;
	uint16_t reject_id = 0;

	uint16_t netshort;

	// check the opening delimiter
	memcpy(&netshort, buf, 2);
	if (ntohs(netshort) != DELIMITER) {
		return ERR_OPEN_DELIMITER;
	}

	// copy the client id
	memcpy(&client_id, &buf[2], 1);

	// copy the type
	memcpy(&netshort, &buf[3], 2);
	type = ntohs(netshort);

	// check the type, populate as necessary
	int close_addr;
	if (type == ACK) {
		ack_packet *ap = (ack_packet*) p;

		// copy segment id
		memcpy(&segment_id, &buf[5], 1);
		// set close addr to check closing delimiter
		close_addr = 6;

		ap->client_id = client_id;
		ap->type = type;
		ap->segment_id = segment_id;

	} else if (type == REJECT) {
		reject_packet *rp = (reject_packet*) p;

		// copy reject id
		memcpy(&netshort, &buf[5], 2);
		reject_id = ntohs(netshort);
		// copy segment id
		memcpy(&segment_id, &buf[7], 1);
		// set close addr to check closing delimiter
		close_addr = 8;

		rp->client_id = client_id;
		rp->type = type;
		rp->reject_id = reject_id;
		rp->segment_id = segment_id;
	} else if (type == DATA) {
		data_packet *dp = (data_packet*) p;

		// copy segment id
		memcpy(&segment_id, &buf[5], 1);
		// copy the length
		memcpy(&len, &buf[6], 1);
		// copy the payload
		payload = malloc(sizeof(uint8_t) * len);
		memcpy(payload, &buf[7], len);
		// set close addr to check closing delimiter
		close_addr = 7 + len;

		dp->client_id = client_id;
		dp->type = type;
		dp->segment_id = segment_id;
		dp->len = len;
		dp->payload = payload;

		// scan payload for end delimiter
		char *delimiter_ptr = strstr((char*)payload, DELIMITER_STR);
		if (delimiter_ptr) {
			return ERR_LEN_MISMATCH;
		}
	} else {
		// invalid format
		return ERR_INVALID_FMT;
	}

	// check the closing delimiter
	memcpy(&netshort, &buf[close_addr], 2);

	if (ntohs(netshort) != DELIMITER) {
		return ERR_CLOSE_DELIMITER;
	}

	return SUCCESS;
}

// void ct_init(client_table* const ct) {
// 	memset(ct->ptrs, 0, sizeof(ct->ptrs));
// 	memset(ct->buf, 0, sizeof(ct->buf));
// }
// 
// int ct_exists(client_table const *ct, uint8_t id) {
// 	return ct->ptrs[id] == NULL;
// }
// 
// void ct_create(client_table* const ct, uint8_t id) {
// 	// buf is memset to 0, and client's ze
// 	ct->ptrs[id] = &ct->buf[id];
// }
// 
// void ct_increment(client_table* const ct, uint8_t id) {
// 	(ct->buf[id]).segment_no += 1;
// }
// 
// void ct_delete(client_table* const ct, uint8_t id) {
// 	ct->ptrs[id] = NULL;
// 	memset(&ct->buf[id], 0, sizeof(client));
// }
// 
// int ct_expected_segment_no(client_table* const ct, uint8_t id) {
// 	if (!ct_exists(ct, id)) {
// 		ct_create(ct, id);
// 	}
// 
// 	return ct->buf[id].segment_no;
// }

//packet_list *create_packet_list() {
//	packet_list *pl = malloc(sizeof(packet_list));
//	packet_list_node *buf = malloc(
//			sizeof(packet_list_node)*PACKET_LIST_INITIAL_SIZE
//	);
//
//	pl->head = NULL;
//	pl->tail = NULL;
//	pl->buf = buf;
//	pl->size = PACKET_LIST_INITIAL_SIZE;
//
//	return pl;
//}
//
//void free_packet(packet *p) {
//	free(p->payload);
//}
//
//void free_packet_list(packet_list *pl) {
//	if (pl->head) {
//		size_t increment = sizeof(packet_list_node);
//		packet_list_node *cursor = pl->buf;
//		do {
//			free_packet(cursor->p);
//			cursor += increment;
//		} while(cursor <= pl->tail);
//	}
//	free(pl->buf);
//	free(pl);
//}
//
//void append(packet_list *pl, packet *p) {
//	size_t packet_size = sizeof(packet);
//	size_t packet_list_node_size = sizeof(packet_list_node);
//
//	// initial list
//	if (pl->tail == NULL) {
//		pl->head = pl->buf;
//		pl->tail = pl->buf;
//	}
//
//	// number of elements in the list
//	size_t num = ((pl->head - pl->tail) / packet_size) + 1;
//
//	// our buffer is full, resize
//	if (num >= pl->size) {
//		packet_list_node *buf = malloc(
//				packet_list_node_size * PACKET_LIST_FACTOR * pl->size
//		);
//
//		memcpy(buf, pl->buf, packet_list_node_size * pl->size);
//
//		free(pl->buf);
//
//		pl->buf = buf;
//		pl->head = buf;
//		pl->tail = buf + packet_list_node_size * pl->size;
//	}
//
//		//memcpy(pl->buf, p, packet_size);
//	//memcpy(tail, 
//}
