#ifndef COMMON_H
#define COMMON_H

#include "penplotter.h"
#include "serial.h"

#include <ESP32Servo.h>

#ifndef PENPLOTTER_DEVICE_MASTER
#ifndef PENPLOTTER_DEVICE_CLIENT
#error "Must define PENPLOTTER_DEVICE_MASTER or PENPLOTTER_DEVICE_CLIENT"
#endif
#endif

#ifdef PENPLOTTER_DEVICE_MASTER
#ifdef PENPLOTTER_DEVICE_CLIENT
#error "Device cannot be master and client at the same time"
#endif
#endif

#ifdef PENPLOTTER_DEVICE_MASTER
#pragma message("Pen plotter info: This compilation targets master devices")
#define PENPLOTTER_DEVICE_STRING "master"
#endif
#ifdef PENPLOTTER_DEVICE_CLIENT
#pragma message("Pen plotter info: This compilation targets client devices")
#define PENPLOTTER_DEVICE_STRING "client"
#endif

#define PIN_MOTOR_BACKWARD 19
#define PIN_MOTOR_FORWARD 22
#define PIN_POTENTIOMETER 33
#define PIN_SERVO 25


struct common_motor_state {
  bool forward;
  bool backward;
};

struct common_axis_calibration_data {
  uint16_t min;
  uint16_t max;
};

struct common_callback_state {
  volatile bool calibration_ack;
  volatile bool actuator_ack;
  volatile bool servo_up_ack;
  volatile bool servo_down_ack;
};

struct common_state {
  struct common_axis_calibration_data motor_calibration;
  struct serial_state serial;
  struct common_callback_state cb;
  Servo *servo;
};

void common_setup(struct common_state *state);

/* Calibrate local actuator. This function is blocking. */
void common_actuator_calibrate(struct common_axis_calibration_data *calibration);
/* Calibrate remote actuator. This function is asynchronous.
 * Call common_actuator_calibrate_wait_remote to block until complete. */
void common_actuator_calibrate_remote(void);
/* Block until remote actuator completes calibration.
 * Caller must call common_actuator_calibrate_remote before calling this
 * function or this will block indefinitely. */
void common_actuator_calibrate_wait_remote(struct common_state *state);

/* Move actuator to specified position. x and y is 1016dpi (HPGL) value.
 * This function is blocking. */
void common_actuator_move_pos(struct common_state *state, uint16_t x, uint16_t y);
/* Move actuator to specified value. value is 1016dpi (HPGL) value.
 * This function is blocking. */
void common_actuator_move(struct common_axis_calibration_data *calibration,
                          uint16_t value);

enum penplotter_status common_println(const char *line);
enum penplotter_status common_printf(const char *format, ...);

#define common_print_debug(str) \
    common_printf("%s: " str "\n", __func__);
#define common_print_debugf(fmt, ...) \
    common_printf("%s: " fmt, __func__, ##__VA_ARGS__)

#endif /* COMMON_H */
