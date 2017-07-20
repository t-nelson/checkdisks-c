
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct age_unit
{
  uint64_t    upper_bound;
  const char* unit;
};

static const struct age_unit age_units[] =
{
  { 60,         "s" },
  { 60,         "m" },
  { 24,         "h" },
  {  7,         "d" },
  { 52,         "w" },
  { INT64_MAX,  "y" },
};

int timespan_to_string(char** s, int64_t seconds)
{
  const size_t                    n_units     = ARRAY_SIZE(age_units);
  const struct  age_unit  const * first_unit  = &age_units[0];
  const struct  age_unit  const * last_unit   = &age_units[n_units - 1];
  const struct  age_unit*         prev_unit   = NULL;
  const struct  age_unit*         unit;
        int64_t                   prev_val    = 0;
        int64_t                   cur_val     = seconds;
        int                       rc          = -1;

  for (unit = first_unit; unit != last_unit; unit++)
  {
    if (cur_val < unit->upper_bound)
      break;

    prev_val  =   cur_val % unit->upper_bound;
    cur_val   /=  unit->upper_bound;
    prev_unit =   unit;
  }

  if (first_unit == unit || (last_unit == unit && cur_val >= 100))
    rc = asprintf(s, "%zu%s", cur_val, unit->unit);
  else
    rc = asprintf(s, "%zu%s%2zu%s", cur_val, unit->unit, prev_val, prev_unit->unit);

  return rc;
}

struct size_fmt
{
  double      upper_bound;
  const char* fmt;
};

static  const char* size_prefixes[] =
{ 
  " ", "k", "M", "G", "T", "P", "E"
};

#if 1
#define B ""
#else
#define B "B"
#endif

static  const struct size_fmt size_fmts[] =
{
  {    1.0, "%4.3f%s" B },
  {   10.0, "%4.2f%s" B },
  {  100.0, "%4.1f%s" B },
  { 1000.0, "%3.0f.%s" B },
};

int bytes_to_string(char** s, size_t bytes)
{
  const size_t            n_prefixes    = ARRAY_SIZE(size_prefixes);
  const size_t            n_fmts        = ARRAY_SIZE(size_fmts);
  const char**            first_prefix  = &size_prefixes[0];
  const char**            last_prefix   = &size_prefixes[n_prefixes - 1];
  const struct size_fmt*  first_fmt     = &size_fmts[0];
  const struct size_fmt*  last_fmt      = &size_fmts[n_fmts - 1];
        int               rc            = -1;
        double            size          = bytes;
  const char**            prefix        = NULL;

  for (prefix = first_prefix; prefix < last_prefix; prefix++)
  {
    if (size < 1000.0)
      break;
    size /= 1000.0;
  }

  if (first_prefix == prefix)
    rc = asprintf(s, "%4lu" B, (uint64_t)size);
  else
  {
    const struct size_fmt*  fmt;
          double            trunc;

    for (fmt = first_fmt; fmt < last_fmt; fmt++)
    {
      if (size < fmt->upper_bound)
        break;
    }

    trunc = 1000.0 / fmt->upper_bound;
    rc    = asprintf(s, fmt->fmt, floor(size * trunc) / trunc, *prefix);
  }

  return rc;
}

