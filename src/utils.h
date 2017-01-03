
#ifndef UTILS_H__
#define UTILS_H__

#include <stdlib.h>
#include <stdint.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAX(a,b)      ((a) > (b) ? (a) : (b))

int
timespan_to_string(
  char**  s
, int64_t seconds
);
int
bytes_to_string(
  char** s
, size_t bytes
);

#endif /* UTIL_H__ */
