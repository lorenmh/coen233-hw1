#ifndef __TYPES__
#define __TYPES__

#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

#define DELIMETER 0xFFFF

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

#endif
