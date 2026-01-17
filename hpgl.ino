#include "hpgl.h"

void hpgl_setup(struct hpgl_state *hpgl) {
  hpgl_command_end(hpgl);
}

void hpgl_axis_complete(struct hpgl_state *hpgl) {
  common_print_debugf("move axis %d %d\n", hpgl->x, hpgl->y);

  common_actuator_move_pos(hpgl->common, hpgl->x, hpgl->y);

  hpgl->x = 0;
  hpgl->y = 0;
}

enum penplotter_status hpgl_arg(struct hpgl_state *hpgl, char *input) {
  if (strcmp(hpgl->current_command, "SP") == 0) {
    /* select pen */
    common_print_debugf("select pen %s\n", input);

    hpgl->state = HPGL_STATE_EXPECT_END;
    return SUCCESS;
  }

  return ERR_UNIMPLEMENTED;
}

enum penplotter_status hpgl_set_command(struct hpgl_state *hpgl, char *input) {
  enum penplotter_status ret = ERR_UNIMPLEMENTED;

  if (strcmp(input, "IN") == 0) {
    /* init */
    common_print_debug("HPGL init");

    hpgl->state = HPGL_STATE_EXPECT_END;
    ret = SUCCESS;
  } else if (strcmp(input, "PD") == 0) {
    /* pen down */
    common_print_debug("pen down");

    common_servo_down(hpgl->common);

    hpgl->state = HPGL_STATE_EXPECT_AXIS_X;
    ret = SUCCESS;
  } else if (strcmp(input, "PU") == 0) {
    /* pen up */
    common_print_debug("pen up");

    common_servo_up(hpgl->common);

    hpgl->state = HPGL_STATE_EXPECT_AXIS_X;
    ret = SUCCESS;
  } else if (strcmp(input, "SP") == 0) {
    /* select pen */
    hpgl->state = HPGL_STATE_EXPECT_ARG;
    ret = SUCCESS;
  }

  if (ret >= 0) {
    memcpy(hpgl->current_command, input, 2);
    hpgl->current_command[2] = 0;
  } else {
    hpgl->current_command[0] = 0;
  }

  return ret;
}

enum penplotter_status hpgl_push(struct hpgl_state *hpgl, char *input) {
  if (!hpgl->current_command[0]) {
    enum penplotter_status ret;
    char command[3];

    memcpy(command, input, 2);
    command[2] = 0;

    if ((ret = hpgl_set_command(hpgl, command)) < 0) {
      return ret;
    }

    if (strlen(input + 2) != 0) {
      return hpgl_push(hpgl, input + 2);
    }

    return SUCCESS;
  }

  unsigned long i;
  switch (hpgl->state) {
    case HPGL_STATE_UNKNOWN:
      return SUCCESS;

    case HPGL_STATE_EXPECT_END:
      common_print_debug("expect end assert");
      return ERR_ASSERT;

    case HPGL_STATE_EXPECT_AXIS_X:
      i = strtoul(input, NULL, 10);
      hpgl->x = (uint16_t)i;
      hpgl->state = HPGL_STATE_EXPECT_AXIS_Y;

      return SUCCESS;
    case HPGL_STATE_EXPECT_AXIS_Y:
      i = strtoul(input, NULL, 10);
      hpgl->y = (uint16_t)i;
      hpgl->state = HPGL_STATE_EXPECT_AXIS_X;

      hpgl_axis_complete(hpgl);
      return SUCCESS;

    case HPGL_STATE_EXPECT_ARG:
      return hpgl_arg(hpgl, input);
  }
}

void hpgl_command_end(struct hpgl_state *hpgl) {
  hpgl->current_command[0] = 0;
  hpgl->state = HPGL_STATE_UNKNOWN;
}
