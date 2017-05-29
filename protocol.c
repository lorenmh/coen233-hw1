#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include "protocol.h"

int ptob(packet *p, uint8_t *buf) {
	memset(buf, 0xff, 2);
	buf[2] = p->client_id;
	uint16_t netshort = htons(p->type);
	memcpy(&buf[3], &netshort, 2);

	if (p->type == REJECT) {
		reject_packet *rp = (reject_packet*) p;

		netshort = htons(rp->reject_id);
		memcpy(&buf[5], &netshort, 2);

		buf[7] = rp->segment_id;

		memset(&buf[8], 0xff, 2);

		return 10;
	}

	if (p->type == ACK) {
		ack_packet *ap = (ack_packet*) ap;

		buf[5] = ap->segment_id;

		memset(&buf[6], 0xff, 2);

		for (int i = 0; i < 8; i++) {
			printf("\\x%02x", buf[i]);
		}
		printf("\n");
		return 8;
	}

	data_packet *dp = (data_packet*) p;
	buf[5] = dp->segment_id;
	buf[6] = dp->len;
	memcpy(&buf[7], dp->payload, dp->len);
	memset(&buf[dp->len + 7], 0xff, 2);
	return dp->len + 9;
}

/*============================================================================
 * protocol.c:
 * -----------
 * Contains utility functions for implementing the protcol.
=============================================================================*/

//int ptob(packet *p, uint8_t* const buf) {
//}

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
void ptos(packet *p, parser_return_t rt, char *str) {
	char *resolution_str = "";
	if (rt == ERR_OPEN_DELIMITER) {
		resolution_str = "PARSER ERROR: ERR_OPEN_DELIMITER\n";
	} else if (rt == ERR_CLOSE_DELIMITER) {
		resolution_str = "PARSER ERROR: ERR_CLOSE_DELIMITER\n";
	} else if (rt == ERR_LEN_MISMATCH) {
		resolution_str = "PARSER ERROR: ERR_LEN_MISMATCH\n";
	} else if (rt == ERR_INVALID_FMT) {
		resolution_str = "PARSER ERROR: ERR_INVALID_FMT\n";
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
				str,
				"%s"
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
				str,
				"%s"
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
			reject_str = "UNDEFINEDE_REJECT_ID";
		}

		sprintf(
				str,
				"%s"
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
 * parser_return_t prt: the parser return type for the client packet
 * packet* const rp: a pointer to the response packet which will be populated
 *   with data
 * uint8_t* const client_table: a table of expected segment numbers for clients
 * FILE* verification_db: a pointer to the Verification_Database.txt file
=============================================================================*/
void resolve_response_packet(
		packet const *req_p,
		parser_return_t const req_prt,
		packet* const res_p,
		uint8_t* const client_table,
		FILE* const verification_db) {

	// only server can send ACK/REJECT, handles parse errors
	if (req_p->type == ACK || req_p->type == REJECT || req_prt != SUCCESS) {
		reject_packet *res_rp = (reject_packet*) res_p;

		// END_PACKET_MISSING reject packet
		if (req_p->type == DATA && req_prt == ERR_CLOSE_DELIMITER) {
			data_packet *req_dp = (data_packet*) req_p;

			res_rp->client_id = req_dp->client_id;
			res_rp->type = REJECT;
			res_rp->reject_id = END_PACKET_MISSING;
			res_rp->segment_id = req_dp->segment_id;
			return;
		}

		// LEN_MISMATCH reject packet
		if (req_p->type == DATA && req_prt == ERR_LEN_MISMATCH) {
			data_packet *req_dp = (data_packet*) req_p;

			res_rp->client_id = req_dp->client_id;
			res_rp->type = REJECT;
			res_rp->reject_id = LEN_MISMATCH;
			res_rp->segment_id = req_dp->segment_id;
			return;
		}

		// generic reject response (non protocol defined errors);
		res_rp->client_id = req_p->client_id;
		res_rp->type = REJECT;
		res_rp->reject_id = 0; // undefined behavior, non protocol defined errors
		res_rp->segment_id = 0; // undefined error, might not have seg id, set 0
		return;
	}

	// we are dealing with a data packet
	data_packet *req_dp = (data_packet*) req_p;
	uint8_t client_id = req_dp->client_id;
	uint8_t segment_id = req_dp->segment_id;

	uint8_t expected_segment_id = client_table[client_id];

	if (segment_id > expected_segment_id) {
		reject_packet *res_rp = (reject_packet*) res_p;

		res_rp->client_id = client_id;
		res_rp->type = REJECT;
		res_rp->reject_id = OUT_OF_SEQ;
		res_rp->segment_id = segment_id;
		return;
	}


	if (segment_id < expected_segment_id) {
		reject_packet *res_rp = (reject_packet*) res_p;

		res_rp->client_id = client_id;
		res_rp->type = REJECT;
		res_rp->reject_id = DUP_PACKET;
		res_rp->segment_id = segment_id;
		return;
	}

	// increment expected segment id in client table
	client_table[client_id] += 1;

	if (req_dp->type == DATA) {
		ack_packet *res_ap = (ack_packet*) res_p;
		res_ap->client_id = client_id;
		res_ap->type = ACK;
		res_ap->segment_id = segment_id;
	}

}

parser_return_t parse_packet_buf(
		uint8_t const *buf,
		void* const p) {

	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
	uint8_t len = 0;
	uint8_t *payload = 0;
	uint16_t reject_id = 0;

	uint16_t hostshort;

	// check the opening delimiter
	memcpy(&hostshort, buf, 2);
	if (ntohs(hostshort) != DELIMITER) {
		return ERR_OPEN_DELIMITER;
	}

	// copy the client id
	memcpy(&client_id, &buf[2], 1);

	// copy the type
	memcpy(&hostshort, &buf[3], 2);
	type = ntohs(hostshort);

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
		memcpy(&hostshort, &buf[5], 2);
		reject_id = ntohs(hostshort);
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
		// null terminate the payload so we can print it as a string easily
		payload[len] = 0x0;
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
	memcpy(&hostshort, &buf[close_addr], 2);

	if (ntohs(hostshort) != DELIMITER) {
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
