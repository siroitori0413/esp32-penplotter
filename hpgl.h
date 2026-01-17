#ifndef HPGL_H
#define HPGL_H

#include "penplotter.h"
#include "common.h"

enum hpgl_internal_state {
  HPGL_STATE_UNKNOWN,
  HPGL_STATE_EXPECT_END,
  HPGL_STATE_EXPECT_AXIS_X,
  HPGL_STATE_EXPECT_AXIS_Y,
  HPGL_STATE_EXPECT_ARG,
};

struct hpgl_state {
  struct common_state *common;

  /* Command currently active, e.g. "PD\0"
   * If not set, first byte should be NULL byte */
  char current_command[3];
  /* State of current command. */
  enum hpgl_internal_state state;

  /* X state for HPGL_STATE_EXPECT_AXIS_X */
  uint16_t x;
  /* Y state for HPGL_STATE_EXPECT_AXIS_Y */
  uint16_t y;
};

void hpgl_setup(struct hpgl_state *hpgl);

/* Push HPGL commands and parameters.
 * hpgl_push expects string input split at comma and expects hpgl_command_end
 * called at semicolon correctly.
 * For example, HPGL data
 * "PD0,0,100,100;PU;"
 * must call HPGL functions in following sequence:
 * ```
 * hpgl_push "PD0";
 * hpgl_push "0";
 * hpgl_push "100";
 * hpgl_push "100";
 * hpgl_command_end;
 * hpgl_push "PU";
 * hpgl_command_end;
 * ``` */
enum penplotter_status hpgl_push(struct hpgl_state *hpgl, char *input);
/* Reset current command state. */
void hpgl_command_end(struct hpgl_state *hpgl);

#endif /* HPGL_H */
