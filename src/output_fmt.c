
#include "output_fmt.h"
#include "scsi_target.h"
#include "blk_dev_info.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#define NA_     "N/A"
#define NA_LEN  (sizeof(NA_) - 1)

const char const* NA  = NA_;

enum COLOR
{
    _COLOR_FIRST  = 0
  , COLOR_NONE
                  = _COLOR_FIRST
  , COLOR_RED
  , COLOR_GREEN
  , COLOR_YELLOW
  , COLOR_BLUE
  , COLOR_MAGENTA
  , COLOR_CYAN
  , COLOR_BOLD
  , _COLOR_MAX
};

static const char*  colors[_COLOR_MAX] =
{
    [COLOR_RED]     = "\x1B[31m"
  , [COLOR_GREEN]   = "\x1B[32m"
  , [COLOR_YELLOW]  = "\x1B[33m"
  , [COLOR_BLUE]    = "\x1B[34m"
  , [COLOR_MAGENTA] = "\x1B[35m"
  , [COLOR_CYAN]    = "\x1B[36m"
  , [COLOR_BOLD]    = "\x1B[1m"
  , [COLOR_NONE]    = "\x1B[0m"
};

enum FIELD
{
    _FLD_FIRST      = 0
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

struct field
{
  char*       string;
  int         len;
  enum COLOR  color;
  bool        bold;
};

typedef int
(*get_field)(
        struct field* f
, const void*         arg
);

static int
get_field_port_id(
        struct field* f
, const void*         arg
)
{
  const char* port_id = arg;
        int   rc      = -1;
  
  if (-1 != (rc = asprintf(&f->string, "%s", port_id)))
    f->len = rc;

  return rc;
}

static int
get_field_blk_dev(
        struct field* f
, const void*         arg
)
{
  const struct scsi_target* st  = arg;
        int                 rc  = -1;

  if (st)
    rc = asprintf(&f->string, "sd%s", scsi_target_get_blk_dev(st));
  else
  {
    if (-1 != (rc = asprintf(&f->string, "N/C")))
      f->bold = true;
  }

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_model(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info*  bdi = arg;
        int                   rc  = -1;

  if (bdi->model != BLK_DEV_INFO_UNSET_STRING)
    rc = asprintf(&f->string, "%s", bdi->model);
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_serial(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info*  bdi = arg;
        int                   rc  = -1;

  if (bdi->serial != BLK_DEV_INFO_UNSET_STRING)
    rc = asprintf(&f->string, "%s", bdi->serial);
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_size(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info*  bdi = arg;
        int                   rc  = -1;

  if (bdi->size_byt != BLK_DEV_INFO_UNSET_U64)
    rc = bytes_to_string(&f->string, bdi->size_byt);
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_age(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info*  bdi = arg;
        int                   rc  = -1;

  if (bdi->smt_pwr_on_sec != BLK_DEV_INFO_UNSET_I64)
    rc = timespan_to_string(&f->string, bdi->smt_pwr_on_sec);
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_temp(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info* bdi = arg;
        int                   rc  = -1;

  if (bdi->smt_temp_kel != BLK_DEV_INFO_UNSET_DBL)
  {
    double  temp_C  = (bdi->smt_temp_kel - 273.0);
    double  temp_F  = temp_C * 9.0 / 5.0 + 32.0;
    if (-1 != (rc = asprintf(&f->string, "%.1lfF", temp_F)))
    {
      if (      temp_F > 120.0)
      {
        f->color  = COLOR_RED;
        f->bold   = true;
      }
      else if ( temp_F > 110.0)
        f->color  = COLOR_YELLOW;
      else
        f->color  = COLOR_GREEN;
    }
  }
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

static int
get_field_bad_sect(
        struct field* f
, const void*         arg
)
{
  const struct blk_dev_info* bdi  = arg;
        int                   rc  = -1;

  if (bdi->smt_bad_sect != BLK_DEV_INFO_UNSET_I64)
  {
    if (-1 != (rc = asprintf(&f->string, "%ld", bdi->smt_bad_sect)))
    {
      if (      bdi->smt_bad_sect > 1)
      {
        f->color  = COLOR_RED;
        f->bold   = true;
      }
      else if ( bdi->smt_bad_sect > 0)
        f->color  = COLOR_YELLOW;
    }
  }
  else
    rc = asprintf(&f->string, "%s", NA);

  if (-1 != rc)
    f->len = rc;

  return rc;
}

enum JUSTIFY
{
    _JUST_FIRST = 0
  , JUST_RIGHT
                = _JUST_FIRST
  , JUST_LEFT
  , JUST_CENTER
  , _JUST_MAX
};

struct field_def
{
  const char*   label;
  enum JUSTIFY  justify;
  get_field     get_fn;

  int           max_len;
};

struct field_def  fields[] =
{
  [FLD_PORT_ID]     = { "Port",       JUST_RIGHT,   get_field_port_id   },
  [FLD_BLK_DEV]     = { "Device",     JUST_RIGHT,   get_field_blk_dev   },
  [FLD_MODEL_NO]    = { "Model #",    JUST_LEFT,    get_field_model     },
  [FLD_SERIAL_NO]   = { "Serial #",   JUST_LEFT,    get_field_serial    },
  [FLD_SIZE_BYTES]  = { "Size",       JUST_RIGHT,   get_field_size      },
  [FLD_PWR_ON_HRS]  = { "Age",        JUST_RIGHT,   get_field_age       },
  [FLD_TEMP_mC]     = { "Temp.",      JUST_RIGHT,   get_field_temp      },
  [FLD_BAD_SECT]    = { "Bad Sect.",  JUST_CENTER,  get_field_bad_sect  },
};

struct disk_info
{
  struct disk_info* next;
  struct field      fields[_FLD_MAX];
};

void disk_info_print(const struct disk_info* di)
{
  size_t  fld;

  for (fld = 0; fld < _FLD_MAX; fld++)
  {
    const struct field* f       = &di->fields[fld];
    const char*         val     = f->string;
    const char*         color   = "";
    const char*         uncolor = "";
    const char*         bold    = "";
    const int           max     = fields[fld].max_len;
    const int           len     = f->len;
          int           lpad    = 0;
          int           rpad    = 0;

    if (f->color != COLOR_NONE)
    {
      color   = colors[f->color];
      uncolor = colors[COLOR_NONE];
    }

    if (f->bold)
    {
      bold    = colors[COLOR_BOLD];
      uncolor = colors[COLOR_NONE];
    }

    switch (fields[fld].justify)
    {
      case JUST_RIGHT:
        lpad = max - len;
        break;
      case JUST_LEFT:
        rpad = max - len;
        break;
      case JUST_CENTER:
        lpad = (max - len) / 2;
        rpad = (max - len) - lpad;
        break;
      case _JUST_MAX:
        break;
    }

    printf("%*s%s%s%s%s%*s  ", lpad, "", bold, color, val ? val : NA, uncolor, rpad, "");
  }

  putchar('\n');
}

struct disk_info* disk_info_create()
{
  struct disk_info* di;
  if (NULL != (di = calloc(1, sizeof(struct disk_info))))
  {
    size_t i;
    for (i = 0; i < _FLD_MAX; i++)
    {
      di->fields[i].len   = NA_LEN;
      di->fields[i].color = COLOR_NONE;
      di->fields[i].bold  = false;
    }
  }
  return di;
}

#define FIELD_FREE(f) if (f != NA) free(f)

void disk_info_destroy(struct disk_info* di)
{
  if (di)
  {
    size_t fld;

    for (fld = 0; fld < _FLD_MAX; fld++)
      FIELD_FREE(di->fields[fld].string);
    free(di);
  }
}

#define SET_FIELD(fld, pdi, pval)                                   \
  do {                                                              \
    int len;                                                        \
    if (-1 != (len = fields[fld].get_fn(&pdi->fields[fld], pval)))  \
      fields[fld].max_len = MAX(fields[fld].max_len, len);          \
  } while (0)

static struct disk_info* disks  = NULL;

void disks_append_disk(struct disk_info* di)
{
  struct disk_info* p = disks;

  while (p)
  {
    if (!p->next)
      break;
    p = p->next;
  }

  if (p)
  {
    di->next  = NULL;
    p->next   = di;
  }
  else
  {
    di->next  = NULL;
    disks     = di;
  }
}

void
output_fmt_init(
)
{
  size_t i;

  for (i = 0; i < _FLD_MAX; i++)
    fields[i].max_len = MAX(NA_LEN, strlen(fields[i].label));
}

void
output_fmt_deinit(
)
{
  while (disks)
  {
    struct disk_info* next = disks->next;

    disk_info_destroy(disks);
    disks = next;
  }
}

void
output_fmt_print(
)
{
  size_t i;

  printf("%s", colors[COLOR_BOLD]);
  for (i = 0; i < _FLD_MAX; i++)
    printf("%-*s  ", fields[i].max_len, fields[i].label);
  printf("%s\n", colors[COLOR_NONE]);

  while (disks)
  {
    struct disk_info* next = disks->next;
    disk_info_print(disks);
    disks = next;
  }
}

bool
output_fmt_add_dev(
  const char*               port_id
, const struct scsi_target* st
)
{
  struct disk_info* di;
  bool              rc  = false;

  if (NULL != (di = disk_info_create()))
  {
    rc = true;
    
    SET_FIELD(FLD_PORT_ID, di, port_id);
    SET_FIELD(FLD_BLK_DEV, di, st);

    if (st)
    {
      const struct blk_dev_info* bdi;

      if (NULL != (bdi = scsi_target_get_blk_dev_info(st)))
      {
        SET_FIELD(FLD_MODEL_NO,   di, bdi);
        SET_FIELD(FLD_SERIAL_NO,  di, bdi);
        SET_FIELD(FLD_SIZE_BYTES, di, bdi);
        SET_FIELD(FLD_PWR_ON_HRS, di, bdi);
        SET_FIELD(FLD_TEMP_mC,    di, bdi);
        SET_FIELD(FLD_BAD_SECT,   di, bdi);
      }
    }
    disks_append_disk(di);
  }

  return rc;
}

