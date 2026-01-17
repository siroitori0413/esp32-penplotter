#ifndef SERIAL_H
#define SERIAL_H

#include "penplotter.h"

#ifdef PENPLOTTER_DEVICE_MASTER
#define PIN_SERIAL_TX 26
#define PIN_SERIAL_RX 32
#endif
#ifdef PENPLOTTER_DEVICE_CLIENT
#define PIN_SERIAL_TX 32
#define PIN_SERIAL_RX 26
#endif

#define SERIAL_MAX_PACKET_LEN 16
#define SERIAL_MAX_PACKET_DATA_LEN \
    (SERIAL_MAX_PACKET_LEN - sizeof(struct serial_packet))

enum serial_packet_type {
  PACKET_TYPE_EXTENDED,
  PACKET_TYPE_SERIAL_WRITEOUT,
  PACKET_TYPE_ACTUATOR_MOTION,
};

enum serial_packet_type_extended {
  /* Initiate calibration. */
  PACKET_TYPE_EXTENDED_CALIBRATION,
  /* Calibration complete. */
  PACKET_TYPE_EXTENDED_CALIBRATION_ACK,
  /* Actuator moved to specified value. */
  PACKET_TYPE_EXTENDED_ACTUATOR_MOTION_OK,
  /* Pen down. */
  PACKET_TYPE_EXTENDED_SERVO_DOWN,
  /* Pen down acknowledged. */
  PACKET_TYPE_EXTENDED_SERVO_DOWN_ACK,
  /* Pen up. */
  PACKET_TYPE_EXTENDED_SERVO_UP,
  /* Pen up acknowledged. */
  PACKET_TYPE_EXTENDED_SERVO_UP_ACK,
};

/* Base data type used to transfer data, such as events and position data over
 * software serial configured on glove port. */
struct serial_packet {
  uint8_t type;
  uint8_t size;
  uint8_t checksum;
  uint8_t data[];
} __attribute__((packed));

/* Extended packet is packet without any value. This is mainly used for events,
 * such as acknowledge packet. */
struct serial_data_extended {
  uint8_t extended_type;
} __attribute__((packed));

/* Print "data" contents to USB serial. "data" must be null terminated. */
struct serial_data_serial_writeout {
  char data[SERIAL_MAX_PACKET_DATA_LEN];
} __attribute__((packed));

/* Move actuator to "value". */
struct serial_data_actuator_motion {
  uint16_t value;
} __attribute__((packed));

struct serial_state {
  struct common_state *common;
  struct serial_state_callback {
    void (*calibration_ack)(serial_state *state);
    void (*actuator_motion_ack)(serial_state *state);
    void (*servo_up_ack)(serial_state *state);
    void (*servo_down_ack)(serial_state *state);
  } callback;
};

void serial_setup();
void serial_send(struct serial_packet *packet);

/* Process buffered incoming serial data. This should be non-blocking. */
enum penplotter_status serial_update(struct serial_callback *cb);

#endif /* SERIAL_H */
