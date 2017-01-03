
#ifndef OUTPUT_FMT_H__
#define OUTPUT_FMT_H__

#include <stdbool.h>

struct scsi_target;

void
output_fmt_init(
);

void
output_fmt_print(
);

bool
output_fmt_add_dev(
  const char*               port_id
, const struct scsi_target* st
);

void
output_fmt_deinit(
);

#endif /* OUTPUT_FMT_H__ */

