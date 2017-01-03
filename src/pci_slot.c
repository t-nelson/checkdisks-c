
#include "pci_slot.h"
#include "scsi_host.h"
#include "scsi_target.h"
#include <stdlib.h>

struct pci_slot
{
  struct pci_slot*  next;
  char*             address;
  struct scsi_host* hosts;
};

/* pci_addr should be on the heap */
struct pci_slot*
pci_slot_create(
    char* pci_addr
)
{
  struct pci_slot* ps;
  
  if (NULL != (ps = calloc(1, sizeof(struct pci_slot))))
    ps->address = pci_addr;

  return ps;
}

struct scsi_host*
pci_slot_get_first_host(
    const struct pci_slot*  ps
)
{
  return ps ? ps->hosts : NULL;
}

void
pci_slot_add_sibling(
          struct pci_slot*  ps
  ,       struct pci_slot*  prev
  ,       struct pci_slot** head
)
{
  if (prev)
  {
    ps->next    = prev->next;
    prev->next  = ps;
  }
  else
  {
    ps->next    = *head;
    *head       = ps;
  }
}

struct pci_slot*
pci_slot_next(
    const struct pci_slot* ps
)
{
  return ps ? ps->next : NULL;
}

void
pci_slot_destroy(
    struct pci_slot* ps
)
{
  if (ps)
  {
    struct scsi_host* host = ps->hosts;
    while (host)
    {
      struct scsi_host* next = scsi_host_next(host);
      scsi_host_destroy(host);
      host = next;
    }
    free(ps->address);
    free(ps);
  }
}

struct scsi_host*
pci_slot_find_host(
    const struct pci_slot*  ps
  , int                     host_no
)
{
  struct scsi_host* sh = ps->hosts;

  while (sh)
  {
    int sh_host_no  = scsi_host_get_host_no(sh);
    if (sh_host_no == host_no)
      return sh;
    sh = scsi_host_next(sh);
  }

  return NULL;
}

void
pci_slot_add_host(
    struct pci_slot*  ps
  , struct scsi_host* sh
)
{
  struct scsi_host*   host        = ps->hosts;
  struct scsi_host*   prev        = NULL;
  struct scsi_target* sh_targets  = scsi_host_get_first_target(sh);
  int                 sh_score    = scsi_host_get_host_no(sh) * 1000;

  if (sh_targets)
  {
    const struct scsi_tuple* sh_t0_tup = scsi_target_get_tuple(sh_targets);
    sh_score += sh_t0_tup->target;
  }

  while (host)
  {
    struct scsi_target* host_targets  = scsi_host_get_first_target(host);
    int                 host_score    = scsi_host_get_host_no(host) * 1000;

    if (host_targets)
    {
      const struct scsi_tuple* host_t0_tup;
      host_t0_tup =   scsi_target_get_tuple(host_targets);
      host_score  +=  host_t0_tup->target;
    }

    if (sh_score < host_score)
      break;
    prev = host;
    host = scsi_host_next(host);
  }
  
  scsi_host_add_sibling(sh, prev, &ps->hosts);
}

const char*
pci_slot_get_address(
    const struct pci_slot*  ps
)
{
  return ps ? ps->address : NULL;
}

