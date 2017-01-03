
#include "scsi_host.h"
#include "scsi_target.h"
#include <stdlib.h>
#include <string.h>

struct scsi_host
{
  struct scsi_host*   next;
  struct pci_slot*    pci_slot;
  int                 host_no;
  struct scsi_target* targets;
};

struct scsi_host*
scsi_host_create(
    int               host_no
  , struct pci_slot*  ps
)
{
  struct scsi_host* sh;
  if (NULL != (sh = calloc(1, sizeof(struct scsi_host))))
  {
    sh->host_no   = host_no;
    sh->pci_slot  = ps;
  }
  return sh;
}

void
scsi_host_add_sibling(
        struct scsi_host*     sh
  ,     struct scsi_host*     prev
  ,     struct scsi_host**    head
)
{
  if (prev)
  {
    sh->next    = prev->next;
    prev->next  = sh;
  }
  else
  {
    sh->next  = *head;
    *head     = sh;
  }
}

struct scsi_host*
scsi_host_next(
    const struct scsi_host* sh
)
{
  return sh ? sh->next : NULL;
}

void
scsi_host_destroy(
    struct scsi_host* sh
)
{
  if (sh)
  {
    struct scsi_target* t = scsi_host_get_first_target(sh);
    while (t)
    {
      struct scsi_target* next  = scsi_target_next(t);
      scsi_target_destroy(t);
      t = next;
    }
    free(sh);
  }
}

void
scsi_host_add_target(
    struct scsi_host*   sh
  , struct scsi_target* st
)
{
        struct scsi_target* prev      = NULL;
        struct scsi_target* target    = sh->targets;
  const struct scsi_tuple*  st_tuple  = scsi_target_get_tuple(st);

  while (target)
  {
    const struct scsi_tuple*  target_tuple  = scsi_target_get_tuple(target);

    if (st_tuple->target < target_tuple->target)
      break;
    prev    = target;
    target  = scsi_target_next(target);
  }

  scsi_target_add_sibling(st, prev, &sh->targets);
}

struct scsi_target*
scsi_host_find_target(
    const struct scsi_host*   sh
  , const struct scsi_tuple*  tup
)
{
  struct scsi_target* st  = sh->targets;

  while (st)
  {
    const struct scsi_tuple* st_tup = scsi_target_get_tuple(st);
    if (0 == memcmp(tup, st_tup, sizeof(struct scsi_tuple)))
      break;
    st = scsi_target_next(st);
  }

  return st;
}

struct scsi_target*
scsi_host_get_first_target(
    const struct scsi_host* sh
)
{
  return sh ? sh->targets : NULL;
}

int
scsi_host_get_host_no(
    const struct scsi_host*   sh
)
{
  return sh ? sh->host_no : SCSI_HOST_INVALID;
}

