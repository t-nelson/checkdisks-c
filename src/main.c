
#include "sysfs_scan.h"
#include "info_manager.h"
#include "pci_slot.h"
#include "scsi_host.h"
#include "scsi_target.h"
#include "udisks2.h"
#include "output_fmt.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <pci/pci.h>

enum ADAPTER_FIELDS
{
    _AD_FLD_FIRST   = 0
  , AD_FLD_NAME
                    = _AD_FLD_FIRST
  , AD_FLD_CLASS
#if 1
  , AD_FLD_VEN_DEV
#else
  , AD_FLD_VENDOR
  , AD_FLD_DEVICE
#endif
  , _AD_FLD_MAX
};

static const char* adapter_headers[_AD_FLD_MAX] =
{
    [AD_FLD_NAME]     = "Name"
  , [AD_FLD_CLASS]    = "Type"
#if 1
  , [AD_FLD_VEN_DEV]  = "Vendor / Device"
#else
  , [AD_FLD_VENDOR]   = "Vendor"
  , [AD_FLD_DEVICE]   = "Device"
#endif
};

static int adapter_lens[_AD_FLD_MAX] = {};

struct adapter_info
{
  struct adapter_info*  next;
  char*                 fields[_AD_FLD_MAX];
};

static struct adapter_info* g_ai = NULL;

struct adapter_info*
adapter_info_create(
)
{
  return calloc(1, sizeof(struct adapter_info));
}

void adapter_info_append(struct adapter_info* ai, struct adapter_info** head)
{
  struct adapter_info*  p = *head;

  while (p)
  {
    if (!p->next)
      break;
    p = p->next;
  }

  if (p)
  {
    ai->next  = NULL;
    p->next   = ai;
  }
  else
  {
    ai->next  = NULL;
    *head     = ai;
  }
}

void adapter_info_print(const struct adapter_info* head)
{
  const struct adapter_info*  p = head;

  if (p)
  {
    size_t i;
    for (i = _AD_FLD_FIRST; i < _AD_FLD_MAX; i++)
    {
      adapter_lens[i] = MAX(adapter_lens[i], strlen(adapter_headers[i]));
      printf("%-*s  ", adapter_lens[i], adapter_headers[i]);
    }
    putchar('\n');
    while (p)
    {
      for (i = _AD_FLD_FIRST; i < _AD_FLD_MAX; i++)
        printf("%-*s  ", adapter_lens[i], p->fields[i]);
      putchar('\n');
      p = p->next;
    }
  }
}

void adapter_info_destroy(struct adapter_info* ai)
{
  while (ai)
  {
    struct adapter_info*  next = ai->next;
    size_t                i;
    for (i = _AD_FLD_FIRST; i < _AD_FLD_MAX; i++)
      free(ai->fields[i]);
    free(ai);
    ai = next;
  }
}
  
void fill_output()
{
  const struct pci_slot*      ps          = info_get_first_pci_slot();
        int                   slot_no     = 0;
        char                  port_id[32];
        struct pci_access*    pa;

  if (NULL != (pa = pci_alloc()))
  {
    char              buf[1024] = {};
    struct pci_filter pf        = {};

    pci_init(pa);
    pci_scan_bus(pa);

    while (ps)
    {
      struct scsi_host* sh      = pci_slot_get_first_host(ps);
      int               host_no = 0;
      struct pci_dev*   pd;
      char*             pci_addr;

      if (NULL != (pci_addr = strdup(pci_slot_get_address(ps))))
      {
        pci_filter_init(pa, &pf);
        pci_filter_parse_slot(&pf, pci_addr);
        if (NULL != (pd = pci_get_dev(pa, pf.domain, pf.bus, pf.slot, pf.func)))
        {
          struct adapter_info* nai;

          if (NULL != (nai = adapter_info_create()))
          {
            int rc;

            pci_fill_info(pd, PCI_FILL_IDENT | PCI_FILL_CLASS);
            
            if (-1 != (rc = asprintf(&nai->fields[AD_FLD_NAME], "c%d", slot_no)))
              adapter_lens[AD_FLD_NAME] = MAX(adapter_lens[AD_FLD_NAME], rc);
            pci_lookup_name(pa, buf, sizeof(buf), PCI_LOOKUP_CLASS, pd->device_class);
            if (-1 != (rc = asprintf(&nai->fields[AD_FLD_CLASS], "%s", buf)))
              adapter_lens[AD_FLD_CLASS] = MAX(adapter_lens[AD_FLD_CLASS], rc);
#if 1
            pci_lookup_name(pa, buf, sizeof(buf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, pd->vendor_id, pd->device_id);
            if (-1 != (rc = asprintf(&nai->fields[AD_FLD_VEN_DEV], "%s", buf)))
              adapter_lens[AD_FLD_VEN_DEV] = MAX(adapter_lens[AD_FLD_VEN_DEV], rc);
#else
            pci_lookup_name(pa, buf, sizeof(buf), PCI_LOOKUP_VENDOR, pd->vendor_id);
            if (-1 != (rc = asprintf(&nai->fields[AD_FLD_VENDOR], "%s", buf)))
              adapter_lens[AD_FLD_VENDOR] = MAX(adapter_lens[AD_FLD_VENDOR], rc);
            pci_lookup_name(pa, buf, sizeof(buf), PCI_LOOKUP_DEVICE | PCI_LOOKUP_NO_NUMBERS, pd->device_id);
            if (-1 != (rc = asprintf(&nai->fields[AD_FLD_DEVICE], "%s", buf)))
              adapter_lens[AD_FLD_DEVICE] = MAX(adapter_lens[AD_FLD_DEVICE], rc);
#endif

            adapter_info_append(nai, &g_ai);
          }
          pci_free_dev(pd);
        }
        free(pci_addr);
      }

      while (sh)
      {
        struct scsi_target* st  = scsi_host_get_first_target(sh);
        if (!st)
        {
          snprintf(port_id, sizeof(port_id), "c%dp%d.x", slot_no, host_no);
          output_fmt_add_dev(port_id, NULL);
        }
        while (st)
        {
          const struct scsi_tuple* st_tup = scsi_target_get_tuple(st);
          snprintf(port_id, sizeof(port_id), "c%dp%d.%d", slot_no, host_no, st_tup->target);
          output_fmt_add_dev(port_id, st);
          st = scsi_target_next(st);
        } 
        sh = scsi_host_next(sh);
        host_no++;
      }
      ps = pci_slot_next(ps);
      slot_no++;
    }
    pci_cleanup(pa);

  }
}

static const struct option  longopts[]  =
{
    { "help",     no_argument,        NULL, 'h' }
  , { "reverse",  no_argument,        NULL, 'r' }
  , { "sort",     required_argument,  NULL, 's' }
  , {}
};

static const char* sort_opt_strings[] =
{
    [FLD_PORT_ID]     = "port"
  , [FLD_BLK_DEV]     = "dev"
  , [FLD_MODEL_NO]    = "model"
  , [FLD_SERIAL_NO]   = "serial"
  , [FLD_SIZE_BYTES]  = "size"
  , [FLD_PWR_ON_HRS]  = "age"
  , [FLD_TEMP_mC]     = "temp"
  , [FLD_BAD_SECT]    = "sect"
};

static void print_help(const char* argv0)
{
  char*       tmp       = strdup(argv0);
  const char* progname;
  size_t  i;

  if (tmp)
  {
    char* slash = strrchr(tmp, '/');
    if (slash)
      progname = slash + 1;
    else
      progname = tmp;
  }
  else
    progname = argv0;

  printf("Usage: %s [OPTION]...\n\n", progname);
  printf(
      "Mandatory arguments to long options are also mandatory for short options.\n"
      " -h  --help        Display this help message.\n"
      " -r  --reverse     Reverse sort order.\n"
      " -s  --sort=<ARG>  Sort output by ARG. Accepted values for ARG are: \n");
  printf(
      "                   %s", sort_opt_strings[_FLD_FIRST]);
  for (i = _FLD_FIRST + 1; i < _FLD_MAX; i++)
    printf(", %s", sort_opt_strings[i]);
  putchar('\n');

  free(tmp);
}

int main(int argc, char* argv[])
{
  enum FIELD  sort_fld  = FLD_INVALID;
  int         sort_dir  = 1;
  int         opt;

  while (-1 != (opt = getopt_long(argc, argv, "hrs:", longopts, NULL)))
  {
    switch (opt)
    {
      case 'r':
        sort_dir = -1;
        break;
      case 's':
      {
        enum FIELD f;
        for (f = _FLD_FIRST; f < _FLD_MAX; f++)
        {
          if (0 == strcmp(optarg, sort_opt_strings[f]))
          {
            sort_fld = f;
            break;
          }
        }
        if (FLD_INVALID != sort_fld)
          break;
        /* else fall through on bad param */
        printf("Unrecognized parameter '%s' for option -s/--sort.\n", optarg);
      }
      case '?':
      case 'h':
      default:
        print_help(argv[0]);
        exit(1);
        break;
    }
  }

  if (udisks2_init())
  {
    sysfs_init();
    sysfs_scan();
    output_fmt_init();
    fill_output();
    output_fmt_sort(sort_fld, sort_dir);
    printf("  --- Disks ---\n");
    output_fmt_print();
    output_fmt_deinit();
    printf("  --- Adapters ---\n");
    adapter_info_print(g_ai);
    adapter_info_destroy(g_ai);

    udisks2_deinit();
  }

  info_cleanup();
  
  return 0;
}

