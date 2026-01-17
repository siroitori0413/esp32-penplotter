#include "penplotter.h"
#include "serial.h"
#include "common.h"

void serial_setup() {
  Serial2.begin(115200, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  Serial2.setTimeout(50);
}

void serial_send(struct serial_packet *packet) {
  packet->checksum = serial_packet_checksum(packet);

  Serial2.write((const uint8_t *)packet,
                sizeof(struct serial_packet) + (size_t)packet->size);
}

uint8_t serial_packet_checksum(struct serial_packet *packet) {
  uint8_t checksum = 0xFF;

  for (uint8_t i = 0; i < packet->size; i++) {
    checksum = checksum ^
      *((uint8_t *)packet) + sizeof(struct serial_packet) + i;
  }

  return checksum;
}

bool serial_packet_checksum_valid(struct serial_packet *packet) {
  uint8_t checksum = serial_packet_checksum(packet);
  if (checksum == packet->checksum) return true;

  common_print_debugf("checksum mismatch! %d != %d\n",
                      checksum, packet->checksum);

  return false;
}

enum penplotter_status
serial_process_packet_extended(struct serial_data_extended *data,
                               struct serial_state *state) {
  enum penplotter_status ret = SUCCESS;

  switch (data->extended_type) {
    case PACKET_TYPE_EXTENDED_CALIBRATION:
      common_print_debug("calibration request");

      common_actuator_calibrate(&state->common->motor_calibration);
      serial_send_extended(PACKET_TYPE_EXTENDED_CALIBRATION_ACK);

      break;
    case PACKET_TYPE_EXTENDED_SERVO_DOWN:
      common_servo_down(state->common);
      serial_send_extended(PACKET_TYPE_EXTENDED_SERVO_DOWN_ACK);

      break;
    case PACKET_TYPE_EXTENDED_SERVO_UP:
      common_servo_up(state->common);
      serial_send_extended(PACKET_TYPE_EXTENDED_SERVO_UP_ACK);

      break;
    case PACKET_TYPE_EXTENDED_CALIBRATION_ACK:
      if (state->callback.calibration_ack)
        state->callback.calibration_ack(state);

      break;
    case PACKET_TYPE_EXTENDED_ACTUATOR_MOTION_OK:
      if (state->callback.actuator_motion_ack)
        state->callback.actuator_motion_ack(state);

      break;
    case PACKET_TYPE_EXTENDED_SERVO_DOWN_ACK:
      if (state->callback.servo_down_ack)
        state->callback.servo_down_ack(state);

      break;
    case PACKET_TYPE_EXTENDED_SERVO_UP_ACK:
      if (state->callback.servo_up_ack)
        state->callback.servo_up_ack(state);

      break;

    default:
      ret = ERR_UNIMPLEMENTED;
  }

err:
  return ret;
}

enum penplotter_status serial_process_packet(struct serial_packet *packet,
                                             struct serial_state *state) {
  enum penplotter_status ret = SUCCESS;

  if (!serial_packet_checksum_valid(packet)) {
    ret = ERR_ASSERT;
    goto err;
  }

  switch (packet->type) {
    case PACKET_TYPE_EXTENDED:
      ret = serial_process_packet_extended((struct serial_data_extended *)packet->data,
                                           state);
      break;
    case PACKET_TYPE_SERIAL_WRITEOUT:
      Serial.print(((struct serial_data_serial_writeout *)packet->data)->data);
      break;
    case PACKET_TYPE_ACTUATOR_MOTION:
      common_actuator_move(&state->common->motor_calibration,
                           ((struct serial_data_actuator_motion *)packet->data)->value);
      serial_send_extended(PACKET_TYPE_EXTENDED_ACTUATOR_MOTION_OK);

      break;
    default:
      ret = ERR_UNIMPLEMENTED;
  }

err:
  return ret;
}

enum penplotter_status serial_update(struct serial_state *state) {
  enum penplotter_status ret;
  uint8_t packet[SERIAL_MAX_PACKET_LEN];
  size_t read_len;

  if (Serial2.available() == 0) {
    ret = ERR_DEFER;
    goto err;
  }

  read_len = Serial2.readBytes(packet, sizeof(struct serial_packet));

  if (read_len != sizeof(struct serial_packet)) {
    ret = ERR_DEFER;
    goto err;
  }

  if (((struct serial_packet *)&packet)->size > SERIAL_MAX_PACKET_DATA_LEN) {
    ret = ERR_ASSERT;
    goto err;
  }

  read_len = Serial2.readBytes(packet + sizeof(struct serial_packet),
                               ((struct serial_packet *)&packet)->size);
  if (read_len != ((struct serial_packet *)packet)->size) {
    ret = ERR_ASSERT;
    goto err;
  }

  ret = serial_process_packet((struct serial_packet *)&packet, state);

err:
  return ret;
}

void serial_send_extended(enum serial_packet_type_extended type) {
  const size_t packet_size = 
    sizeof(struct serial_packet) + sizeof(struct serial_data_extended);

  uint8_t raw_packet[packet_size];

  struct serial_packet *packet = (struct serial_packet *)raw_packet;
  struct serial_data_extended *data = (struct serial_data_extended *)packet->data;

  packet->type = PACKET_TYPE_EXTENDED;
  packet->size = sizeof(struct serial_data_extended);
  data->extended_type = type;

  common_printf("sending extended packet %d\n", type);

  serial_send(packet);
}
