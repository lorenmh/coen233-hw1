#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>

typedef enum ptype {
  DATA = 0xFFF1,
  ACK = 0xFFF2,
  REJECT = 0xFFF3
} ptype;

typedef enum rtype {
  OUT_OF_SEQ = 0xFFF4,
  LEN_MISMATCH = 0xFFF5,
  END_PACKET_MISSING = 0xFFF6,
  DUP_PACKET = 0xFFF7
} rtype;

typedef struct packet_header {
  uint8_t client_id;
  uint16_t type;
} packet_header;

typedef struct packet_data_body {
  uint8_t pid;
  uint8_t len;
  uint8_t payload;
} packet_data_body;

typedef struct packet_ack_body {
  uint8_t pid;
} packet_ack_body;

typedef struct packet_rej_body {
  uint16_t rid;
  uint8_t pid;
} packet_rej_body;

int main(int argc, char *argv[]) {
  unsigned char buf[] = {
    0,
    0xFF,
    0xF2,
    0x00
  };

  packet_header header;
  memcpy(&header, buf, 3);

  printf("%d\n", header.type);
}
