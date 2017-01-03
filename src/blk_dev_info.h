
#ifndef BLK_DEV_INFO_H__
#define BKL_DEV_INFO_H__

#include <stdlib.h>
#include <stdint.h>

#define BLK_DEV_INFO_UNSET_STRING (NULL)
#define BLK_DEV_INFO_UNSET_U64    (UINT64_MAX)
#define BLK_DEV_INFO_UNSET_I64    (-1)
#define BLK_DEV_INFO_UNSET_DBL    (-1.0)

struct blk_dev_info
{
  char*     model;
  char*     serial;
  uint64_t  size_byt;
  int64_t   smt_pwr_on_sec;
  double    smt_temp_kel;
  int64_t   smt_bad_sect;
};

#define BLK_DEV_INFO_STATIC_INIT  {                           \
                                    BLK_DEV_INFO_UNSET_STRING \
                                  , BLK_DEV_INFO_UNSET_STRING \
                                  , BLK_DEV_INFO_UNSET_U64    \
                                  , BLK_DEV_INFO_UNSET_I64    \
                                  , BLK_DEV_INFO_UNSET_DBL    \
                                  , BLK_DEV_INFO_UNSET_I64    \
                                  } 
struct blk_dev_info*
blk_dev_info_create(
);
void
blk_dev_info_destroy(
          struct blk_dev_info*  bdi
);

/* blk_dev_info_init is called automatically by blk_dev_info_create */
void
blk_dev_info_init(
          struct blk_dev_info*  bdi
);
/* blk_dev_info_cleanup is called automatically by blk_dev_info_destroy */
void
blk_dev_info_cleanup(
          struct blk_dev_info*  bdi
);

void
blk_dev_info_dump(
    const struct blk_dev_info*  bdi
  ,       int                   indent
);

#endif /* BLK_DEV_INFO_H__ */

