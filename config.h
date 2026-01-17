/* Master is the device should have USB connected and client has servo.
   Master handles Y axis, client handles XZ. */
// #define PENPLOTTER_DEVICE_MASTER
// #define PENPLOTTER_DEVICE_CLIENT

/* Actuator range of motion in mm */
#define PENPLOTTER_X_MM 45
#define PENPLOTTER_Y_MM 45

/* Wi-Fi configurations */
#define WIFI_SSID ""
#define WIFI_KEY ""

/* Aliases; do not change */
#ifdef PENPLOTTER_DEVICE_MASTER
#define PENPLOTTER_ACTUATOR_RANGE_MM PENPLOTTER_Y_MM
#endif
#ifdef PENPLOTTER_DEVICE_CLIENT
#define PENPLOTTER_ACTUATOR_RANGE_MM PENPLOTTER_X_MM
#endif
