#include <stdlib.h>

#include "protocol.h"

// Assume that buf has been allocated enough space, otherwise segfault
int resolve_packet(char const *buf, packet* p) {
  uint8_t client_id;
  uint16_t type;
  uint8_t segment_id;
  uint8_t len = 0;
  uint8_t *payload = 0;
  uint8_t reject_id = 0;
  
  uint16_t netshort;

  // check the opening delimiter
  memcpy(&netshort, buf, 2);
  if (ntohs(netshort) != DELIMETER) {
    return ERR_OPEN_DELIMETER;
  }

  // copy the client id
  memcpy(&client_id, &buf[2], 1);

  // copy the type
  memcpy(&netshort, &buf[3], 2);
  type = ntohs(netshort);

  // check the type, populate as necessary
  int close_addr;
  if (type == ACK) {
    // copy segment id
    memcpy(&segment_id, &buf[5], 1);
    // set close addr to check closing delimiter
    close_addr = 8;
  } else if (type == REJECT) {
    // copy reject id
    memcpy(&netshort, &buf[5], 2);
    reject_id = ntohs(netshort);
    // copy segment id
    memcpy(&segment_id, &buf[7], 1);
    // set close addr to check closing delimiter
    close_addr = 10;
  } else if (type == DATA) {
    // copy segment id
    memcpy(&segment_id, &buf[5], 1);
    // copy the length
    memcpy(&len, &buf[6], 1);
    // copy the payload
    payload = malloc(sizeof(uint8_t) * len);
    memcpy(payload, &buf[7], len);
    // set close addr to check closing delimiter
    close_addr = 7 + len;
  } else {
    // invalid type
    return ERR_INVALID_TYPE;
  }

  // check the closing delimiter
  memcpy(&netshort, &buf[close_addr], 2);
  if (ntohs(netshort) != DELIMETER) {
    return ERR_CLOSE_DELIMETER;
  }

  p->client_id = client_id;
  p->type = type;
  p->segment_id = segment_id;
  p->len = len;
  p->payload = payload;
  p->reject_id = reject_id;

  return 0;
}
