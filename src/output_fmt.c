
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

enum COLORSET
{
    _COLORSET_FIRST   = 0
  , COLORSET_DEFAULT  = _COLORSET_FIRST
  , COLORSET_NONE
  , _COLORSET_MAX
};

static const char*  colorset[_COLORSET_MAX][_COLOR_MAX] =
{
    [COLORSET_NONE] =
    {
        [COLOR_RED]     = ""
      , [COLOR_GREEN]   = ""
      , [COLOR_YELLOW]  = ""
      , [COLOR_BLUE]    = ""
      , [COLOR_MAGENTA] = ""
      , [COLOR_CYAN]    = ""
      , [COLOR_BOLD]    = ""
      , [COLOR_NONE]    = ""
    }
  , [COLORSET_DEFAULT] =
    {
        [COLOR_RED]     = "\x1B[31m"
      , [COLOR_GREEN]   = "\x1B[32m"
      , [COLOR_YELLOW]  = "\x1B[33m"
      , [COLOR_BLUE]    = "\x1B[34m"
      , [COLOR_MAGENTA] = "\x1B[35m"
      , [COLOR_CYAN]    = "\x1B[36m"
      , [COLOR_BOLD]    = "\x1B[1m"
      , [COLOR_NONE]    = "\x1B[0m"
    }
};

static const char** colors  = colorset[COLORSET_DEFAULT];

struct field
{
  char*       string;
  const void* sort_val;
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
  {
    f->sort_val = f->string;
    f->len      = rc;
  }

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
  {
    if (-1 != (rc = asprintf(&f->string, "sd%s", scsi_target_get_blk_dev(st))))
      f->sort_val = f->string;
  }
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
  {
    if (-1 != (rc = asprintf(&f->string, "%s", bdi->model)))
      f->sort_val = f->string;
  }
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
  {
    f->sort_val = f->string;
    f->len      = rc;
  }

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
  {
    if (-1 != (rc = bytes_to_string(&f->string, bdi->size_byt)))
      f->sort_val = &bdi->size_byt;
  }
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
  {
    if (-1 != (rc = timespan_to_string(&f->string, bdi->smt_pwr_on_sec)))
      f->sort_val = &bdi->smt_pwr_on_sec;
  }
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

      f->sort_val = &bdi->smt_temp_kel;
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

      f->sort_val = &bdi->smt_bad_sect;
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
  [FLD_BLK_DEV]     = { "Dev",        JUST_RIGHT,   get_field_blk_dev   },
  [FLD_MODEL_NO]    = { "Model #",    JUST_LEFT,    get_field_model     },
  [FLD_SERIAL_NO]   = { "Serial #",   JUST_LEFT,    get_field_serial    },
  [FLD_SIZE_BYTES]  = { "Size",       JUST_RIGHT,   get_field_size      },
  [FLD_PWR_ON_HRS]  = { "Age",        JUST_RIGHT,   get_field_age       },
  [FLD_TEMP_mC]     = { "Temp",       JUST_RIGHT,   get_field_temp      },
  [FLD_BAD_SECT]    = { "Bad",        JUST_CENTER,  get_field_bad_sect  },
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
  const struct disk_info* disk = disks;
  size_t                  i;

  printf("%s", colors[COLOR_BOLD]);
  for (i = 0; i < _FLD_MAX; i++)
    printf("%-*s  ", fields[i].max_len, fields[i].label);
  printf("%s\n", colors[COLOR_NONE]);

  while (disk)
  {
    struct disk_info* next = disk->next;
    disk_info_print(disk);
    disk = next;
  }
}

static int64_t disk_info_compare(
  enum FIELD              sort_fld
, int                     sort_dir
, const struct disk_info* ldi
, const struct disk_info* rdi
)
{
  /* sort_dir is used to force the N/A's to the bottom
   * regardless of the user selected sort direction */
  int64_t rc = 0;
  switch (sort_fld)
  {
    case FLD_PORT_ID:
    case FLD_BLK_DEV:
    case FLD_MODEL_NO:
    case FLD_SERIAL_NO:
    {
      const char* lhs = ldi->fields[sort_fld].sort_val;
      const char* rhs = rdi->fields[sort_fld].sort_val;
      if (lhs && rhs)
        rc = strcasecmp(lhs, rhs);
      else if (lhs)
        rc = sort_dir * -1;
      else if (rhs)
        rc = sort_dir * 1;
      break;
    }
    case FLD_SIZE_BYTES:
    {
      const uint64_t* ulhs  = ldi->fields[sort_fld].sort_val;
      const uint64_t* urhs  = rdi->fields[sort_fld].sort_val;
      if (ulhs && urhs)
      {
        const int64_t lhs = (int64_t)MIN(*ulhs, INT64_MAX);
        const int64_t rhs = (int64_t)MIN(*urhs, INT64_MAX);
        rc = lhs - rhs;
      }
      else if (ulhs)
        rc = sort_dir * -1;
      else if (urhs)
        rc = sort_dir * 1;
      break;
    }
    case FLD_PWR_ON_HRS:
    {
      const int64_t*  lhs = ldi->fields[sort_fld].sort_val;
      const int64_t*  rhs = rdi->fields[sort_fld].sort_val;
      if (lhs && rhs)
        rc = *lhs - *rhs;
      else if (lhs)
        rc = sort_dir * -1;
      else if (rhs)
        rc = sort_dir * 1;
      break;
    }
    case FLD_TEMP_mC:
    {
      const double* ulhs  = ldi->fields[sort_fld].sort_val;
      const double* urhs  = rdi->fields[sort_fld].sort_val;
      if (ulhs && urhs)
      {
        const int64_t lhs = (int64_t)MIN(*ulhs*1000.0, (double)INT64_MAX);
        const int64_t rhs = (int64_t)MIN(*urhs*1000.0, (double)INT64_MAX);
        rc = lhs - rhs;
      }
      else if (ulhs)
        rc = sort_dir * -1;
      else if (urhs)
        rc = sort_dir * 1;
      break;
    }
    case FLD_BAD_SECT:
    {
      const int64_t*  lhs = ldi->fields[sort_fld].sort_val;
      const int64_t*  rhs = rdi->fields[sort_fld].sort_val;
      if (lhs && rhs)
        rc = *lhs - *rhs;
      else if (lhs)
        rc = sort_dir * -1;
      else if (rhs)
        rc = sort_dir * 1;
      break;
    }
    default:
      break;
  }
  return rc;
}

void output_fmt_sort(
  enum FIELD  sort_fld
, int         sort_dir
)
{
  if (FLD_INVALID != sort_fld)
  {
    struct disk_info* prev  = NULL;
    struct disk_info* disk  = disks;

    while (disk)
    {
      struct disk_info* n_prev = disk;
      struct disk_info* needle = disk->next;

      while (needle)
      {
        int64_t dir = sort_dir * disk_info_compare(sort_fld, sort_dir, disk, needle);

        /* Try to do something sane with equivalent values */
        if (0 == dir)
          dir = sort_dir * disk_info_compare(FLD_BLK_DEV, sort_dir, disk, needle);
        if (0 == dir)
          dir = sort_dir * disk_info_compare(FLD_PORT_ID, sort_dir, disk, needle);

        if (0 < dir)
        {
          /* remove needle */
          n_prev->next = needle->next;
          /* insert needle at head */
          if (!prev)
            disks = needle;
          else
            prev->next = needle;
          needle->next = disk;
          /* needle is now the min value */
          disk = needle;
        }
        n_prev = needle;
        needle = needle->next;
      }
      prev = disk;
      disk = disk->next;
    }
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

void
output_fmt_enable_colors(
  bool enable
)
{
  enum COLORSET cs  = enable ? COLORSET_DEFAULT : COLORSET_NONE;
  colors = colorset[cs];
}

