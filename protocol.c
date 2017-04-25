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
  if (type == ACK) {
    // copy segment id
    memcpy(&segment_id, &buf[5], 1);
    // check the closing delimiter
    memcpy(&netshort, &buf[8], 2);
    if (ntohs(netshort) != DELIMETER) {
      return ERR_CLOSE_DELIMETER;
    }
  } else if (type == REJECT) {
    // copy reject id
    memcpy(&netshort, &buf[5], 2);
    reject_id = ntohs(netshort);
    // copy segment id
    memcpy(&segment_id, &buf[7], 1);
    // check the closing delimiter
    memcpy(&netshort, &buf[10], 2);
    if (ntohs(netshort) != DELIMETER) {
      return ERR_CLOSE_DELIMETER;
    }
  } else if (type == DATA) {
  } else {
    // invalid type
  }

  p->type = ntohs(netshort);

  return 0;
}
