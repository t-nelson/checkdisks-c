
#include "scsi_target.h"

#include <stdlib.h>
#include <string.h>

struct scsi_target
{
  struct scsi_target*   next;
  struct scsi_host*     scsi_host;
  struct blk_dev_info*  bdi;
  struct scsi_tuple     tuple;
  char                  blk_dev[MAX_BLK_DEV_LEN];
};

struct scsi_target*
scsi_target_create(
          struct scsi_host*     host
  ,       struct blk_dev_info*  bdi
  , const struct scsi_tuple*    tup
  , const char                  blk_dev[MAX_BLK_DEV_LEN]
)
{
  struct scsi_target* new_st;

  if (NULL != (new_st = calloc(1, sizeof(struct scsi_target))))
  {
    new_st->scsi_host = host;
    new_st->bdi       = bdi;
    strncpy(new_st->blk_dev, blk_dev, MAX_BLK_DEV_LEN);
    memcpy(&new_st->tuple, tup, sizeof(struct scsi_tuple));
  }

  return new_st;
}

void
scsi_target_add_sibling(
          struct scsi_target*   st
  ,       struct scsi_target*   prev
  ,       struct scsi_target**  head
)
{
  if (prev)
  {
    st->next    = prev->next;
    prev->next  = st;
  }
  else
  {
    st->next  = *head;
    *head     = st;
  }
}

struct scsi_target*
scsi_target_next(
    const struct scsi_target* st
)
{
  return st ? st->next : NULL;
}

void
scsi_target_destroy(
    struct scsi_target* st
)
{
  free(st);
}

const struct scsi_tuple*
scsi_target_get_tuple(
    const struct scsi_target* st
)
{
  return st ? &st->tuple : NULL;
}

const struct blk_dev_info*
scsi_target_get_blk_dev_info(
    const struct scsi_target* st
)
{
  return st ? st->bdi : NULL;
}

const char*
scsi_target_get_blk_dev(
    const struct scsi_target* st
)
{
  return st ? st->blk_dev : NULL;
}

