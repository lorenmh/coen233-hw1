/*============================================================================
 * protocol.c:
 * -----------
 * Contains utility functions for implementing the protcol.
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include "protocol.h"

/*============================================================================
 * void hexp(...):
 * ---------------
 * Prints the bytes in a buffer in hex format
 * 
 * args:
 * -----
 * uint8_t *buf: a pointer to the buffer to print
 * int n: the number of bytes to print
=============================================================================*/
void hexp(uint8_t *buf, int n) {
	for (int i = 0; i < n; i++) {
		printf("\\x%02x", buf[i]);
	}
	printf("\n");
}

/*============================================================================
 * void qtos(...):
 * ---------------
 * Converts a query (which is in hex byte format) into a set of text strings
 * with radix 10; Ie, given a query of {0x02, 0xf3, 0x42, 0xb2, 0x87} would
 * populate the value '4081234567' into sn_str and '02' into tech_str.
 * 
 * args:
 * -----
 * uint8_t *query: an auth query byte buf
 * char *sn_str: the string to populate subscriber number into
 * char *tech_str: the string to populate tech number into
=============================================================================*/
void qtos(uint8_t *query, char *sn_str, char *tech_str) {
	uint32_t sn_little_endian;
	memcpy(&sn_little_endian, &query[1], 4);
	uint32_t sn = ntohl(sn_little_endian);
	uint8_t tech = query[0];

	sprintf(sn_str, "%010lu", (long) sn);
	sprintf(tech_str, "%02u", tech);
}

/*============================================================================
 * return_t db_query(...):
 * --------------------
 * Performs a lookup in the Verification_Database.txt file, given a subscriber
 * query. Return type specifies whether the lookup was successful or not.
 * 
 * args:
 * -----
 * FILE *fh: file handle / ptr to the Verification_Database.txt file
 * uint8_t *query: an auth query byte buf
=============================================================================*/
return_t db_query(FILE *fh, uint8_t *query) {
	// string segments which will be populated
	char sn_str[11];
	char tech_str[3];

	// for reading lines from the DB
	size_t n = 0;
	char *line = NULL;
	int read;

	// values populated from the DB (for comparison / auth resolution)
	char sn_column[11];
	char tech_column[3];
	char auth_column[2];

	// populate query values into strings
	qtos(query, sn_str, tech_str);

	// seek the beginning of the file
	fseek(fh, 0, SEEK_SET);

	// attempt to find the data in the DB by reading each line of the DB file
	int found = 0;
	while((read = getline(&line, &n, fh) != -1)) {

		// populate column values for comparison
		strncpy(sn_column, line, 10);
		strncpy(tech_column, &line[10], 2);
		strncpy(auth_column, &line[12], 1);

		// null terminate column strings
		sn_column[10] = '\0';
		tech_column[2] = '\0';
		auth_column[1] = '\0';

		// compare subscriber number
		if (!strcmp(sn_str, sn_column)) {
			found = 1;
			// compare tech number
			if (!strcmp(tech_str, tech_column)) {

				// convert auth_column (str) to int
				int auth = atoi(auth_column);

				if (auth) {
					printf(
							"[DB AUTHORIZED]: <%s> <%s> <%s>\n",
							sn_str,
							tech_str,
							auth_column
						);
					free(line);
					return DB_AUTHORIZED;
				} else {
					printf(
							"[DB NOT AUTHORIZED]: <%s> <%s> <%s>\n",
							sn_str,
							tech_str,
							auth_column
						);
					free(line);
					return DB_NOT_AUTHORIZED;
				}
			}
		}
	};

	free(line);

	if (found) {
		// subscriber num was found but wasn't authorized for the queried tech
		printf("[DB NOT AUTHORIZED]: <%s> <%s>\n", sn_str, tech_str);
		return DB_NOT_AUTHORIZED;
	}

	// subscriber not found
	printf("[DB NOT FOUND]: <%s>\n", sn_str);
	return DB_NOT_FOUND;
}

/*============================================================================
 * return_t ptob(...):
 * -------------------
 * Populates a buffer with the packet data; is used for writing packets to a
 * socket. Returns the number of bytes written into the buffer.
 * 
 * args:
 * -----
 * packet *p: The packet which will have its data populated into the buffer
 * uint8_t *buf: the buffer which will have packet data populated into it
=============================================================================*/
int ptob(packet const *p, uint8_t *buf) {
	// write delimiter
	memset(buf, 0xff, 2);
	// set client id
	buf[2] = p->client_id;
	// little endian -> big endian byte
	uint16_t netshort = htons(p->type);
	// copy type into buffer
	memcpy(&buf[3], &netshort, 2);

	// REJECT, ACK, and DATA have different packet sizes

	// write buffer data for REJECT
	if (p->type == REJECT) {
		reject_packet *rp = (reject_packet*) p;

		// reject_id is 2 bytes, need to convert from little endian to big endian
		netshort = htons(rp->reject_id);
		memcpy(&buf[5], &netshort, 2);

		buf[7] = rp->segment_id;

		// write delimiter
		memset(&buf[8], 0xff, 2);

		return 10;
	}

	// write buffer data for ACK
	if (p->type == ACK) {
		ack_packet *ap = (ack_packet*) p;

		buf[5] = ap->segment_id;

		// write delimiter
		memset(&buf[6], 0xff, 2);

		return 8;
	}

	// we are dealing with a data packet of some kind
	//
	data_packet *dp = (data_packet*) p;
	buf[5] = dp->segment_id;
	buf[6] = dp->len;

	// payload is already big endian (net byte order), write straight to buf
	memcpy(&buf[7], dp->payload, dp->len);

	// write delimiter
	memset(&buf[dp->len + 7], 0xff, 2);

	// non-payload size is 9, so size of the buf will be payload_len + 9
	return dp->len + 9;
}

/*============================================================================
 * void ptos(...):
 * ---------------
 * Populates a char* with text information for a given packet.
 * 
 * args:
 * -----
 * void *p: a pointer to a packet
 * return_t rt: The parser return type value (for debugging)
 * char *str: An allocated string in which the text data will be populated
=============================================================================*/
void ptos(packet *p, return_t rt, char *str) {
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
		char payload_s[258]; // null and quotes; 255 + 2 quotes + null term = 258

		if (pt == DATA) {
			// add quotes around payload
			payload_s[0] = '\'';
			memcpy(&payload_s[1], dp->payload, dp->len);
			payload_s[dp->len+1] = '\'';
			payload_s[dp->len+2] = '\0';
		} else {
			// pretty print the subscriber query

			char sn_str[11];
			char tech_str[3];
			qtos(dp->payload, sn_str, tech_str);

			sprintf(
					payload_s,
					"[AUTH PAYLOAD] Tech: %s, Subscriber No: %s",
					tech_str,
					sn_str
			);

			// set the type string
			if (pt == ACC_PER) {
				type_s = "ACC_PER";
			} else if (pt == NOT_PAID) {
				type_s = "NOT_PAID";
			} else if (pt == NOT_EXIST) {
				type_s = "NOT_EXIST";
			} else if (pt == ACCESS_OK) {
				type_s = "ACCESS_OK";
			}
		}

		// write packet information to string
		sprintf(
				str,
				"%s"
				"Client ID: 0x%02x\n"
				"Type: 0x%04x [%s]\n"
				"Segment No: 0x%02x\n"
				"Length: 0x%02x\n"
				"Payload: %s\n",
				resolution_str,
				dp->client_id,
				dp->type,
				type_s,
				dp->segment_id,
				dp->len,
				payload_s
		);
		return;
	}

	// write ACK packet information to string
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

	// write REJECT packet information to string
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
 * verification db and will populate a response packet given this information.
 * 
 * args:
 * -----
 * packet *req_p: a pointer to the request packet
 * return_t req_rt: the parser return type for the request packet
 * packet *res_p: a pointer to the response packet which will be populated
 *   with data
 * uint8_t* client_table: a table of expected segment numbers for clients
 * FILE* db: a pointer to the Verification_Database.txt file
=============================================================================*/
void resolve_response_packet(
		packet const *req_p,
		return_t const req_rt,
		packet* const res_p,
		uint8_t* const client_table,
		FILE* const db) {

	// client can only send types of DATA or ACC_PER
	if ((req_p->type != DATA && req_p->type != ACC_PER) || req_rt != SUCCESS) {
		reject_packet *res_rp = (reject_packet*) res_p;

		// END_PACKET_MISSING reject packet
		if (req_p->type == DATA && req_rt == ERR_CLOSE_DELIMITER) {
			data_packet *req_dp = (data_packet*) req_p;

			res_rp->client_id = req_dp->client_id;
			res_rp->type = REJECT;
			res_rp->reject_id = END_PACKET_MISSING;
			res_rp->segment_id = req_dp->segment_id;
			return;
		}

		// LEN_MISMATCH reject packet
		if (req_p->type == DATA && req_rt == ERR_LEN_MISMATCH) {
			data_packet *req_dp = (data_packet*) req_p;

			res_rp->client_id = req_dp->client_id;
			res_rp->type = REJECT;
			res_rp->reject_id = LEN_MISMATCH;
			res_rp->segment_id = req_dp->segment_id;
			return;
		}

		// generic reject response (non protocol defined errors), for example, if
		// the client sent an ACK request, this is an error but it is not defined
		// by the assignment document.
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

	// perform lookup in client_table
	uint8_t expected_segment_id = client_table[client_id];

	// segment id is out of sequence, reject
	if (segment_id > expected_segment_id) {
		reject_packet *res_rp = (reject_packet*) res_p;

		res_rp->client_id = client_id;
		res_rp->type = REJECT;
		res_rp->reject_id = OUT_OF_SEQ;
		res_rp->segment_id = segment_id;
		return;
	}


	// segment id is duplicate, reject
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

	// if the request is a DATA type, everything is valid, send ACK
	if (req_dp->type == DATA) {
		ack_packet *res_ap = (ack_packet*) res_p;
		res_ap->client_id = client_id;
		res_ap->type = ACK;
		res_ap->segment_id = segment_id;
		return;
	}

	// this is an access permission request (ACC_PER)

	// populate query from payload (payload is a query for ACC_PER requests)
	uint8_t *query = req_dp->payload;

	// perform DB lookup
	return_t db_result = db_query(db, query);

	// set response packet values (doesnt need lookup data yet)
	data_packet *res_dp = (data_packet*) res_p;
	res_dp->client_id = client_id;
	res_dp->segment_id = segment_id;
	res_dp->len = 5;
	res_dp->payload = malloc(5);
	memcpy(res_dp->payload, query, 5);

	// user is authorized, send ACCESS_OK type
	if (db_result == DB_AUTHORIZED) {
		res_dp->type = ACCESS_OK;
		return;
	}

	// user is not authorized, send NOT_PAID type
	if (db_result == DB_NOT_AUTHORIZED) {
		res_dp->type = NOT_PAID;
		return;
	}

	// user wasn't found in DB, send NOT_EXIST type
	if (db_result == DB_NOT_FOUND) {
		res_dp->type = NOT_EXIST;
		return;
	}
}

/*============================================================================
 * return_t parse_packet_buf(...):
 * -------------------------------
 * Parses a buffer of bytes and resolves it into a packet struct
 * 
 * args:
 * -----
 * uint8_t *buf: the buffer with packet data in it
 * int n: the number of bytes to read from the buffer
 * packet *p: the packet which will be populated with the parsed data
=============================================================================*/
return_t parse_packet_buf(
		uint8_t const *buf,
		int const n,
		packet* const p) {

	uint8_t client_id;
	uint16_t type;
	uint8_t segment_id;
	uint8_t len = 0;
	uint8_t *payload = 0;
	uint16_t reject_id = 0;

	int len_mismatch = 0;

	// used for big-endian -> little-endian
	uint16_t hostshort;

	// create a buffer of size n, copy over the data from buf into it. Makes sure
	// that we only get the data we want from buf (won't overflow past n)
	uint8_t packet_buf[n];
	memcpy(&packet_buf, buf, n);

	// check the opening delimiter
	memcpy(&hostshort, packet_buf, 2);
	if (ntohs(hostshort) != DELIMITER) {
		return ERR_OPEN_DELIMITER;
	}

	// copy the client id
	memcpy(&client_id, &packet_buf[2], 1);

	// copy the packet type
	memcpy(&hostshort, &packet_buf[3], 2);
	type = ntohs(hostshort);

	// check the type, populate as necessary
	if (type == ACK) {
		ack_packet *ap = (ack_packet*) p;

		// copy segment id
		memcpy(&segment_id, &packet_buf[5], 1);

		ap->client_id = client_id;
		ap->type = type;
		ap->segment_id = segment_id;

	} else if (type == REJECT) {
		reject_packet *rp = (reject_packet*) p;

		// copy reject id
		memcpy(&hostshort, &packet_buf[5], 2);
		reject_id = ntohs(hostshort);
		// copy segment id
		memcpy(&segment_id, &packet_buf[7], 1);

		rp->client_id = client_id;
		rp->type = type;
		rp->reject_id = reject_id;
		rp->segment_id = segment_id;
	} else if (
			type == DATA || type == ACC_PER || type == NOT_PAID ||
			type == NOT_EXIST || type == ACCESS_OK) {
		data_packet *dp = (data_packet*) p;

		// copy segment id
		memcpy(&segment_id, &packet_buf[5], 1);
		// copy the length
		memcpy(&len, &packet_buf[6], 1);
		// copy the payload
		payload = malloc(sizeof(uint8_t) * len); // memleak
		memcpy(payload, &packet_buf[7], len);
		// null terminate the payload so we can print it as a string easily
		payload[len] = 0x0;

		dp->client_id = client_id;
		dp->type = type;
		dp->segment_id = segment_id;
		dp->len = len;
		dp->payload = payload;

		if (len + 9 != n) {
			len_mismatch = 1;
		}
	} else {
		// invalid format
		return ERR_INVALID_FMT;
	}

	// check the closing delimiter
	memcpy(&hostshort, &packet_buf[n - 2], 2);
	if (ntohs(hostshort) != DELIMITER) {
		return ERR_CLOSE_DELIMITER;
	}

	if (len_mismatch) {
		return ERR_LEN_MISMATCH;
	}

	return SUCCESS;
}
