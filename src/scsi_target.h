
#ifndef SCSI_TARGET_H__
#define SCSI_TARGET_H__

#define MAX_BLK_DEV_LEN (8)

struct scsi_target;
struct scsi_host;
struct blk_dev_info;

struct scsi_tuple
{
  unsigned int          host;
  unsigned int          channel;
  unsigned int          target;
  unsigned int          lun;
};

const struct scsi_tuple*
scsi_target_get_tuple(
    const struct scsi_target* st
);

const struct blk_dev_info*
scsi_target_get_blk_dev_info(
    const struct scsi_target* st
);

struct scsi_target*
scsi_target_create(
          struct scsi_host*     host
  ,       struct blk_dev_info*  bdi
  , const struct scsi_tuple*    tup
  , const char                  blk_dev[MAX_BLK_DEV_LEN]
);
void
scsi_target_destroy(
          struct scsi_target* st
);

void
scsi_target_add_sibling(
          struct scsi_target*   st
  ,       struct scsi_target*   prev
  ,       struct scsi_target**  head
);

struct scsi_target*
scsi_target_next(
    const struct scsi_target* st
);

const char*
scsi_target_get_blk_dev(
    const struct scsi_target* st
);

#endif /* SCSI_TARGET_H__ */

