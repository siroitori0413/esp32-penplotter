#include "common.h"
#include "serial.h"

/*
 * Motor
 */

void common_motor_setup() {
  pinMode(PIN_MOTOR_BACKWARD, OUTPUT);
  pinMode(PIN_MOTOR_FORWARD, OUTPUT);
}

void common_motor_update(struct common_motor_state *data) {
  digitalWrite(PIN_MOTOR_FORWARD, data->forward);
  digitalWrite(PIN_MOTOR_BACKWARD, data->backward);
}

void common_motor_stop() {
  struct common_motor_state state = { .forward = false, .backward = false };

  common_motor_update(&state);
}

/*
 * Potentiometer
 */

void common_potentiometer_setup() {
  pinMode(PIN_POTENTIOMETER, OUTPUT);
}

uint16_t common_potentiometer_read_raw() {
  return analogRead(PIN_POTENTIOMETER);
}

/*
 * Servo
 */

void common_servo_setup(struct common_state *state) {
  pinMode(PIN_SERVO, OUTPUT);

  ESP32PWM::allocateTimer(0);

  state->servo = new Servo();
  state->servo->setPeriodHertz(50);
  state->servo->attach(PIN_SERVO, 500, 2400);
  state->servo->write(90);
}

void common_servo_down(struct common_state *state) {
#ifdef PENPLOTTER_DEVICE_MASTER
  common_servo_down_remote(state);
  return;
#endif /* PENPLOTTER_DEVICE_MASTER */

  state->servo->write(110);
}

void common_servo_up(struct common_state *state) {
#ifdef PENPLOTTER_DEVICE_MASTER
  common_servo_up_remote(state);
  return;
#endif /* PENPLOTTER_DEVICE_MASTER */

  state->servo->write(90);
}

void common_servo_down_remote(struct common_state *state) {
  serial_send_extended(PACKET_TYPE_EXTENDED_SERVO_DOWN);
  while (!state->cb.servo_down_ack) {}

  state->cb.servo_down_ack = false;
}

void common_servo_up_remote(struct common_state *state) {
  serial_send_extended(PACKET_TYPE_EXTENDED_SERVO_UP);
  while (!state->cb.servo_up_ack) {}

  state->cb.servo_up_ack = false;
}

/*
 * Actuator
 */

void common_actuator_calibrate_internal(struct common_motor_state *state,
                                        uint16_t *result) {
  const int32_t max_error = 5;
  int32_t prev_state = (1 << 31) - 1;
  int32_t current_state = 0;

  common_motor_update(state);

  /* Block until potentiometer error < max_error */
  while (abs(prev_state - current_state) > max_error) {
    delay(2500);
    prev_state = current_state;
    current_state = common_potentiometer_read_raw();
  }

  common_motor_stop();

  *result = current_state;
}

void common_actuator_move_pos(struct common_state *state, uint16_t x, uint16_t y) {
#ifdef PENPLOTTER_DEVICE_MASTER
  common_actuator_move_remote(x);
  common_actuator_move(&state->motor_calibration, y);

  common_actuator_wait_remote(state);
#endif /* PENPLOTTER_DEVICE_MASTER */
#ifdef PENPLOTTER_DEVICE_CLIENT
  common_actuator_move_remote(y);
  common_actuator_move(&state->motor_calibration, x);

  common_actuator_wait_remote(state);
#endif /* PENPLOTTER_DEVICE_CLIENT */
}

void common_actuator_move_remote(uint16_t value) {
  const size_t packet_size = sizeof(struct serial_data_actuator_motion)
    + sizeof(struct serial_packet);

  uint8_t raw_packet[packet_size];

  struct serial_packet *packet = (struct serial_packet *)raw_packet;
  struct serial_data_actuator_motion *data =
    (struct serial_data_actuator_motion *)packet->data;

  packet->type = PACKET_TYPE_ACTUATOR_MOTION;
  packet->size = sizeof(struct serial_data_actuator_motion);
  data->value = value;

  serial_send(packet);
}

void common_actuator_wait_remote(struct common_state *state) {
  while (!state->cb.actuator_ack) {}

  state->cb.actuator_ack = false;
}

void common_actuator_move(struct common_axis_calibration_data *calibration,
                          uint16_t value) {
  struct common_motor_state motor = { .forward = false, .backward = false };
  uint16_t target = map(value,
#ifdef PENPLOTTER_DEVICE_MASTER
                        0, PENPLOTTER_ACTUATOR_RANGE_MM * 40,
#endif
#ifdef PENPLOTTER_DEVICE_CLIENT
                        PENPLOTTER_ACTUATOR_RANGE_MM * 40, 0,
#endif
                        calibration->min, calibration->max);
  uint16_t current = common_potentiometer_read_raw();

  common_print_debugf("actuator move, target: %d current: %d\n", target, current);

  if (abs((int32_t)target - (int32_t)current) <= 5) {
    return;
  }

  if (target > current) {
    motor.forward = true;
  } else {
    motor.backward = true;
  }

  common_motor_update(&motor);

  while (
    abs((int32_t)target - (int32_t)current) > 5
      && (motor.forward ? target > current : target < current)
  ) {
    current = common_potentiometer_read_raw();
  }

  motor.forward = true;
  motor.backward = true;
  common_motor_update(&motor);

  delay(10);

  current = common_potentiometer_read_raw();
  common_print_debugf("actuator OK, target: %d current: %d\n", target, current);

  common_motor_stop();
}

void common_actuator_calibrate(struct common_axis_calibration_data *calibration) {
  struct common_motor_state motor;

  motor.forward = false;
  motor.backward = true;
  common_print_debug("calibrate backward");
  common_actuator_calibrate_internal(&motor, &calibration->min);

  motor.forward = true;
  motor.backward = false;
  common_print_debug("calibrate forward");
  common_actuator_calibrate_internal(&motor, &calibration->max);

  common_print_debug("calibrate OK");
}

void common_actuator_calibrate_remote(void) {
  serial_send_extended(PACKET_TYPE_EXTENDED_CALIBRATION);
}

void common_actuator_calibrate_wait_remote(struct common_state *state) {
  while (!state->cb.calibration_ack) {}

  state->cb.calibration_ack = false;
}

void common_actuator_setup(void) {
  common_motor_setup();
  common_potentiometer_setup();
}

void common_send_print_packet(const char *line) {
  const size_t packet_len = sizeof(struct serial_data_serial_writeout)
    + sizeof(struct serial_packet);

  uint8_t raw_data[packet_len];

  struct serial_packet *packet = (struct serial_packet *)raw_data;
  struct serial_data_serial_writeout *data =
    (struct serial_data_serial_writeout *)packet->data;

  size_t len = strlen(line);

  packet->type = PACKET_TYPE_SERIAL_WRITEOUT;

  for (size_t i = 0; i < (len / (SERIAL_MAX_PACKET_DATA_LEN - 1)) + 1; i++) {
    size_t offset = i * (SERIAL_MAX_PACKET_DATA_LEN - 1);
    size_t data_len = min(len - offset, SERIAL_MAX_PACKET_DATA_LEN - 1);
    const char *fragment = line + offset;

    memcpy(data->data, (const void *)fragment, data_len);
    data->data[data_len] = 0;
    packet->size = data_len + 1;

    serial_send(packet);
  }
}

enum penplotter_status common_println(const char *line) {
  enum penplotter_status ret = SUCCESS;
  size_t len = strlen(line);
  char *str = (char *)malloc(len + 16);

  if (snprintf(str, len + 16, "[%s] %s\n",
               PENPLOTTER_DEVICE_STRING, line) < 0) {
    ret = ERR_UNKNOWN;
    goto err;
  }

  Serial.print(str);
  common_send_print_packet(str);

err:
  return ret;
}

enum penplotter_status common_printf(const char *format, ...) {
  enum penplotter_status ret = SUCCESS;
  va_list args;
  char user_str[128];
  char system_str[128 + 16];

  va_start(args, format);

  if (vsnprintf(user_str, 128, format, args) < 0) {
    ret = ERR_UNKNOWN;
    goto err;
  }

  if (snprintf(system_str, 128 + 16, "[%s] %s",
               PENPLOTTER_DEVICE_STRING, user_str) < 0) {
    ret = ERR_UNKNOWN;
    goto err;
  }

  Serial.print(system_str);
  common_send_print_packet(system_str);

err:
  va_end(args);
  return ret;
}

void common_setup(struct common_state *state) {
  Serial.begin(115200);

  serial_setup();
  common_actuator_setup();
  common_servo_setup(state);

  state->serial.callback.calibration_ack = common_cb_calibration_ack;
  state->serial.callback.actuator_motion_ack = common_cb_actuator_ack;
  state->serial.callback.servo_up_ack = common_cb_servo_up_ack;
  state->serial.callback.servo_down_ack = common_cb_servo_down_ack;

  memset((void *)&state->cb, 0, sizeof(struct common_callback_state));

  state->serial.common = state;
}

void common_cb_calibration_ack(struct serial_state *state) {
  state->common->cb.calibration_ack = true;
}

void common_cb_actuator_ack(struct serial_state *state) {
  state->common->cb.actuator_ack = true;
}

void common_cb_servo_up_ack(struct serial_state *state) {
  state->common->cb.servo_up_ack = true;
}

void common_cb_servo_down_ack(struct serial_state *state) {
  state->common->cb.servo_down_ack = true;
}
