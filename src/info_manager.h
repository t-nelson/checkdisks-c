
#ifndef INFO_MANAGER_H__
#define INFO_MANAGER_H__

#include <stdbool.h>

struct pci_slot;

void
info_cleanup(
);

void
info_dump(
);

bool
info_add_host(
    const char* host_path
);

bool
info_add_blk_dev(
    const char* blk_dev_path
);

const struct pci_slot*
info_get_first_pci_slot(
);

#endif /* INFO_MANAGER_H__ */

