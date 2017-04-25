#include "protocol.h"

// Assume that buf has been allocated enough space, otherwise segfault
void resolve_packet_header(const char *buf, packet_header* header) {
  memcpy(&header->client_id, &buf[2], 1);

  uint16_t netshort;
  memcpy(&netshort, &buf[3], 2);

  header->type = ntohs(netshort);
}
