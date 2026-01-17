#ifndef PENPLOTTER_H
#define PENPLOTTER_H

enum penplotter_status {
  SUCCESS = 0,
  ERR_UNKNOWN = -1,
  /* Not implemented. */
  ERR_UNIMPLEMENTED = -2,
  /* Assertion error. */
  ERR_ASSERT = -3,
  /* No data was processed at this time. */
  ERR_DEFER = -4,
};

#endif /* PENPLOTTER_H */
