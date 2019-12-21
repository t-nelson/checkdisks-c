
#include "info_manager.h"
#include "pci_slot.h"
#include "scsi_host.h"
#include "scsi_target.h"
#include "blk_dev_info.h"
#include "udisks2.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct pci_slot* pci_slots = NULL;

void
info_cleanup(
)
{
  struct pci_slot* ps = pci_slots;
  while (ps)
  {
    struct pci_slot* next = pci_slot_next(ps);
    pci_slot_destroy(ps);
    ps = next;
  }
}

void
info_dump(
)
{
  struct pci_slot*  ps      = pci_slots;
  int               slot_no = 0;
  while (ps)
  {
    struct scsi_host* sh      = pci_slot_get_first_host(ps);
    int               host_no = 0;

    while (sh)
    {
      struct scsi_target* st  = scsi_host_get_first_target(sh);
      if (!st)
        printf("c%dp%d.x\tN/C\n", slot_no, host_no);
      while (st)
      {
        const struct blk_dev_info*  bdi         = scsi_target_get_blk_dev_info(st);
        const struct scsi_tuple*    st_tup      = scsi_target_get_tuple(st);
        const char*                 st_blk_dev  = scsi_target_get_blk_dev(st);
        printf("c%dp%d.%d\t(sd%s)\n", slot_no, host_no, st_tup->target, st_blk_dev);
        blk_dev_info_dump(bdi, 2);
        st = scsi_target_next(st);
      } 
      sh = scsi_host_next(sh);
      host_no++;
    }
    ps = pci_slot_next(ps);
    slot_no++;
  }
}

static char*
get_prev_slash(
          char* cur_slash
  , const char* string_start
)
{
  while (--cur_slash > string_start)
  {
    if (*cur_slash == '/')
      return cur_slash;
  }
  return NULL;
}

static bool
parse_host_path(
    const char*   host_path
  ,       int*    host_no
  ,       char**  pci_addr
)
{
  bool  rc    = false;
  char* slash = strrchr(host_path, '/');

  if (slash)
  {
    do
    {
      unsigned int dummy;
      if (1 == sscanf(slash, "/host%u", &dummy))
        *host_no = dummy;
      else if (4 == sscanf(slash, "/%04x:%02x:%02x.%u", &dummy, &dummy, &dummy, &dummy))
      {
        sscanf(slash, "/%12m[.:0-9A-Fa-f]", pci_addr);
        rc = true;
        break;
      }
    } while ((slash = get_prev_slash(slash, host_path)));
  }

  return rc;
}

void
info_add_pci_slot(
    struct pci_slot* ps
)
{
  struct pci_slot*    slot          = pci_slots;
  struct pci_slot*    prev          = NULL;
  struct scsi_host*   ps_host0      = pci_slot_get_first_host(ps);
  struct scsi_target* ps_h0_targets = scsi_host_get_first_target(ps_host0);
  int                 ps_score      = scsi_host_get_host_no(ps_host0) * 1000;

  if (ps_h0_targets)
  {
    const struct scsi_tuple* ps_h0_t0_tup;
    ps_h0_t0_tup  =   scsi_target_get_tuple(ps_h0_targets);
    ps_score      +=  ps_h0_t0_tup->target;
  }

  while (slot)
  {
    struct scsi_host*   slot_host0      = pci_slot_get_first_host(slot);
    struct scsi_target* slot_h0_targets = scsi_host_get_first_target(slot_host0);
    int                 slot_score      = scsi_host_get_host_no(slot_host0) * 1000;

    if (slot_h0_targets)
    {
      const struct scsi_tuple* slot_h0_t0_tup;
      slot_h0_t0_tup  =   scsi_target_get_tuple(slot_h0_targets);
      slot_score      +=  slot_h0_t0_tup->target;
    }

    if (ps_score < slot_score)
      break;

    prev = slot;
    slot = pci_slot_next(slot);
  }

  pci_slot_add_sibling(ps, prev, &pci_slots);
}

static struct pci_slot*
info_find_pci_slot(
    const char* pci_addr
)
{
  struct pci_slot* ps = pci_slots;

  while (ps)
  {
    if (0 == strcmp(pci_addr, pci_slot_get_address(ps)))
      break;
    ps = pci_slot_next(ps);
  }

  return ps;
}

bool
info_add_host(
    const char* host_path
)
{
  int   host_no;
  char* pci_addr;
  bool  rc        = false;

  if (parse_host_path(host_path, &host_no, &pci_addr))
  {
    struct pci_slot* ps;
    struct pci_slot* new_ps = NULL;

    if (NULL == (ps = info_find_pci_slot(pci_addr)))
      ps = new_ps = pci_slot_create(pci_addr);
    else
      free(pci_addr);

    if (ps)
    {
      struct scsi_host* sh  = pci_slot_find_host(ps, host_no);

      if (NULL == sh)
      {
        if (NULL != (sh = scsi_host_create(host_no, ps)))
        {
          pci_slot_add_host(ps, sh);
          if (new_ps)
            info_add_pci_slot(ps);
          rc = true;
        }
      }
    }
  }
  return rc;
}

static bool
parse_blk_dev_path(
    const char*               blk_dev_path
  ,       char**              pci_addr
  ,       int*                host_no
  ,       struct scsi_tuple*  tup
  ,       char                blk_dev[MAX_BLK_DEV_LEN]
)
{
  bool  rc            = false;
  char* new_pci_addr  = NULL;
  char* new_blk_dev   = NULL;
  char* slash         = strrchr(blk_dev_path, '/');

  if (slash)
  {
    unsigned int  dummy[4];
    do
    {
      if (1 == sscanf(slash, "/sd%[^0-9/]", blk_dev))
      {
        strncpy(blk_dev, slash+1, MAX_BLK_DEV_LEN);
      }
      else if (2 == sscanf(slash, "/nvme%un%u", &dummy[0], &dummy[0]))
      {
        strncpy(blk_dev, slash+1, MAX_BLK_DEV_LEN);
      }
      else if (4 == sscanf(slash, "/%u:%u:%u:%u", &dummy[0], &dummy[1], &dummy[2], &dummy[3]))
      {
        tup->host     = dummy[0];
        tup->channel  = dummy[1];
        tup->target   = dummy[2];
        tup->lun      = dummy[3];
      }
      else if (1 == sscanf(slash, "/host%u", &dummy[0]))
        *host_no = dummy[0];
      else if (4 == sscanf(slash, "/%4x:%2x:%2x.%u", &dummy[0], &dummy[1], &dummy[2], &dummy[3]))
      {
        if (1 == sscanf(slash, "/%12m[.:0-9A-Fa-f]", pci_addr))
          new_pci_addr = *pci_addr;
        rc = true;
        break;
      }
    } while ((slash = get_prev_slash(slash, blk_dev_path)));
    if (!rc)
    {
      free(new_blk_dev);
      free(new_pci_addr);
    }
  }

  return rc;
}

#if 0
static struct scsi_host*
info_find_scsi_host(
    int host_no
)
{
  struct pci_slot* ps = pci_slots;

  while (ps)
  {
    struct scsi_host* sh = pci_slot_get_first_host(ps);

    while (sh)
    {
      if (scsi_host_get_host_no(sh) == host_no)
        return sh;
      sh = scsi_host_next(sh);
    }
    ps = pci_slot_next(ps);
  }
  return NULL;
}
#endif

bool
info_add_blk_dev(
    const char* blk_dev_path
)
{
  struct scsi_tuple   tuple                     = {};
  int                 host_no                   = -1;
  bool                rc                        = false;
  char*               pci_addr                  = NULL;
  char                blk_dev[MAX_BLK_DEV_LEN]  = {};

  if (parse_blk_dev_path(blk_dev_path, &pci_addr, &host_no, &tuple, blk_dev))
  {
    struct pci_slot*    ps      = NULL;
    struct pci_slot*    new_ps  = NULL;
    struct scsi_host*   new_sh  = NULL;
    struct scsi_target* new_st  = NULL;

    if (NULL == (ps = info_find_pci_slot(pci_addr)))
      ps = new_ps = pci_slot_create(pci_addr);

    if (ps)
    {
      struct scsi_host* sh  = NULL;

      if (NULL == (sh = pci_slot_find_host(ps, host_no)))
        sh = new_sh = scsi_host_create(host_no, ps);

      if (sh)
      {
        struct scsi_target* st  = NULL;

        if (NULL == (st = scsi_host_find_target(sh, &tuple)))
        {
          struct blk_dev_info* bdi;
          bdi = udisks2_get_blk_dev_smart(blk_dev);
          st = new_st = scsi_target_create(sh, bdi, &tuple, blk_dev);
        }
        
        if (new_st)
          scsi_host_add_target(sh, new_st);
      }

      if (new_sh)
        pci_slot_add_host(ps, new_sh);
    }

    if (new_ps)
      info_add_pci_slot(new_ps);
    else
      free(pci_addr);

    rc = (new_ps || new_sh || new_st);
  }

  return rc;
}

const struct pci_slot*
info_get_first_pci_slot(
)
{
  return pci_slots;
}

