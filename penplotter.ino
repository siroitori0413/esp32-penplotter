#include "config.h"
#include "common.h"
#include "serial.h"
#include "hpgl.h"
#include <WiFi.h>
#include <WiFiServer.h>

#define SERVER_BUFFER_SIZE 128

static struct penplotter_state {
  struct common_state common;
#ifdef PENPLOTTER_DEVICE_MASTER
  WiFiServer *server;
  char server_buffer[SERVER_BUFFER_SIZE];
  uint8_t server_buffer_len;
  hpgl_state hpgl;
#endif
} state;

void setup() {
  // put your setup code here, to run once:

  common_setup(&state.common);
  xTaskCreate(serial_handler, "serial_handler",
              4096, NULL, tskIDLE_PRIORITY, NULL);

  delay(1000); /* Wait for remote serial handler to init */

#ifdef PENPLOTTER_DEVICE_MASTER
  common_actuator_calibrate_remote();
  common_actuator_calibrate(&state.common.motor_calibration);

  common_actuator_calibrate_wait_remote(&state.common);

  WiFi.begin(WIFI_SSID, WIFI_KEY);
  while (WiFi.status() != WL_CONNECTED) {}

  common_printf("localIP: %s\n", WiFi.localIP().toString().c_str());

  state.server = new WiFiServer(23);
  state.server->begin();

  hpgl_setup(&state.hpgl);
  state.hpgl.common = &state.common;
#endif

  common_println("setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:
#ifdef PENPLOTTER_DEVICE_MASTER
  penplotter_status ret;

  WiFiClient client = state.server->available();
  if (!client) {
    return;
  }

  while (client.connected() || client.available() > 0) {
    int size = client.available();
    if (size <= 0) {
      continue;
    }

    uint8_t b = client.read();
    if (b == ';' || b == ',') {
      state.server_buffer[state.server_buffer_len++] = 0;
      common_printf("TCP read: %s\n", state.server_buffer);

      ret = hpgl_push(&state.hpgl, state.server_buffer);
      if (ret < 0) {
        common_printf("hpgl_push: %i\n", ret);
      }

      if (b == ';') {
        hpgl_command_end(&state.hpgl);
      }

      state.server_buffer_len = 0;
      continue;
    }

    state.server_buffer[state.server_buffer_len++] = (char)b;
    if (state.server_buffer_len - 2 >= SERVER_BUFFER_SIZE) {
      common_println("TCP line readout buffer overflow, abort");
      abort();
    }
  }

  client.stop();
  common_println("TCP disconnect");

  hpgl_command_end(&state.hpgl);
#endif
}

void serial_handler(void *) {
  while (1) {
    serial_update(&state.common.serial);
  }
}
