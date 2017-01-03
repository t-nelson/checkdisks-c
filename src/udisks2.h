
#ifndef SMART_SRC_UDISKS2_H__
#define SMART_SRC_UDISKS2_H__

#include <stdbool.h>


struct blk_dev_info;


bool
udisks2_init(
);

void
udisks2_deinit(
);


struct blk_dev_info*
udisks2_get_blk_dev_smart(
    const char* blk_dev
);

#endif /* SMART_SRC_UDISKS2_H__ */

