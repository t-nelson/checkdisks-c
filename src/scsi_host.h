
#ifndef SCSI_HOST_H__
#define SCSI_HOST_H__

struct scsi_host;
struct scsi_target;
struct scsi_tuple;
struct pci_slot;

#define SCSI_HOST_INVALID (-1)

struct scsi_host*
scsi_host_create(
        int                   host_no
  ,     struct pci_slot*      ps
);

void
scsi_host_destroy(
        struct scsi_host*     sh
);


void
scsi_host_add_target(
        struct scsi_host*     sh
  ,     struct scsi_target*   st
);

void
scsi_host_add_sibling(
        struct scsi_host*     sh
  ,     struct scsi_host*     prev
  ,     struct scsi_host**    head
);

struct scsi_host*
scsi_host_next(
    const struct scsi_host*   sh
);

struct scsi_target*
scsi_host_get_first_target(
    const struct scsi_host*   sh
);

struct scsi_target*
scsi_host_find_target(
    const struct scsi_host*   sh
  , const struct scsi_tuple*  tup
);

int
scsi_host_get_host_no(
    const struct scsi_host*   sh
);

#endif /* SCSI_HOST_H__ */

