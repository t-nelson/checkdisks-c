
#include "blk_dev_info.h"

#include <stdio.h>
#include <string.h>

struct blk_dev_info*
blk_dev_info_create(
)
{
  struct blk_dev_info* bdi;

  if (NULL != (bdi = calloc(1, sizeof(struct blk_dev_info))))
    blk_dev_info_init(bdi);

  return bdi;
}

void
blk_dev_info_destroy(
    struct blk_dev_info* bdi
)
{
  blk_dev_info_cleanup(bdi);
  free(bdi);
}

void
blk_dev_info_init(
    struct blk_dev_info* bdi
)
{
  static const struct blk_dev_info template  = BLK_DEV_INFO_STATIC_INIT;

  memcpy(bdi, &template, sizeof(struct blk_dev_info));
}

void
blk_dev_info_cleanup(
    struct blk_dev_info* bdi
)
{
  if (bdi)
  {
    free(bdi->model);
    free(bdi->serial);
  }
}

void
blk_dev_info_dump(
    const struct blk_dev_info*  bdi
  ,       int                   indent
)
{
  printf( "%*.*sModel:       %s\n",  indent, indent, "", bdi->model);
  printf( "%*.*sSerial:      %s\n",  indent, indent, "", bdi->serial);
  printf( "%*.*sSize:        %lu\n", indent, indent, "", bdi->size_byt);
  printf( "%*.*sPwr. on Sec: %ld\n", indent, indent, "", bdi->smt_pwr_on_sec);
  printf( "%*.*sTemp:        %f\n",  indent, indent, "", bdi->smt_temp_kel);
  printf( "%*.*sBad Sect.:   %ld\n", indent, indent, "", bdi->smt_bad_sect);
}

