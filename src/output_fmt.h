
#ifndef OUTPUT_FMT_H__
#define OUTPUT_FMT_H__

#include <stdbool.h>

enum FIELD
{
  FLD_INVALID       = -1
  , _FLD_FIRST      = 0
  , FLD_PORT_ID
                    = _FLD_FIRST
  , FLD_BLK_DEV
  , FLD_MODEL_NO
  , FLD_SERIAL_NO
  , FLD_SIZE_BYTES
  , FLD_PWR_ON_HRS
  , FLD_TEMP_mC
  , FLD_BAD_SECT
  , _FLD_MAX
};

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

