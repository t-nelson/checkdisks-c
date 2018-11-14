
#include "sysfs_scan.h"
#include "info_manager.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define SYSFS_PCI_BASE  "/sys/bus/pci/devices"

static size_t de_size = 0;

typedef bool (*dir_selector_fn)(const char* d_name, const void** next_sel, size_t* n_next_sel);

static bool scsi_dev_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  char  dummy[8]  = {};
  bool  rc        = (1 == sscanf(d_name, "sd%7[a-z]", dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = NULL;
    *n_next_sel = 0;
  }

  return rc;
}

static bool block_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_dev_dir_selector };
                bool            rc                = (0 == strcmp(d_name, "block"));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool scsi_tuple_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { block_dir_selector };
                unsigned int    dummy;
                bool            rc                = (4 == sscanf(d_name, "%u:%u:%u:%u", &dummy, &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool scsi_target_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_tuple_dir_selector };
                unsigned int    dummy;
                bool            rc                = (3 == sscanf(d_name, "target%u:%u:%u", &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool scsi_end_dev_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_target_dir_selector };
                unsigned int    dummy;
                bool            rc                = (2 == sscanf(d_name, "end_device-%u:%u", &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool scsi_port_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_end_dev_dir_selector };
                unsigned int    dummy;
                bool            rc                = (2 == sscanf(d_name, "port-%u:%u", &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool scsi_host_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_target_dir_selector, scsi_port_dir_selector };
                unsigned int    dummy;
                bool            rc                = (1 == sscanf(d_name, "host%u", &dummy));

  if (rc)
  {
    if (next_sel && n_next_sel)
    {
      *next_sel   = next_selectors;
      *n_next_sel = ARRAY_SIZE(next_selectors);
    }
  }

  return rc;
}

static bool ata_dev_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_host_dir_selector };
                unsigned int    dummy;
                bool            rc                = (1 == sscanf(d_name, "ata%u", &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool usb_port_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_host_dir_selector };
                unsigned int    dummy;
                bool            rc                = (4 == sscanf(d_name, "%u-%u:%u.%u", &dummy, &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool usb_hub_port_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { scsi_host_dir_selector };
                unsigned int    dummy;
                bool            rc                = (5 == sscanf(d_name, "%u-%u.%u:%u.%u", &dummy, &dummy, &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool usb_hub_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { usb_hub_port_dir_selector };
                unsigned int    dummy;
                bool            rc                = (3 == sscanf(d_name, "%u-%u.%u", &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool usb_dev_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { usb_port_dir_selector, usb_hub_dir_selector };
                unsigned int    dummy;
                bool            rc                = (2 == sscanf(d_name, "%u-%u", &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool usb_bus_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = { usb_dev_dir_selector };
                unsigned int    dummy;
                bool            rc                = (1 == sscanf(d_name, "usb%u", &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

static bool pci_dev_dir_selector(const char* d_name, const void** next_sel, size_t* n_next_sel)
{
  static const  dir_selector_fn next_selectors[]  = {
                                                      pci_dev_dir_selector
                                                    , ata_dev_dir_selector
                                                    , scsi_host_dir_selector
                                                    , usb_bus_dir_selector
                                                    };
                unsigned int    dummy;
                bool            rc                = (4 == sscanf(d_name, "%4x:%2x:%2x.%x", &dummy, &dummy, &dummy, &dummy));

  if (rc && next_sel && n_next_sel)
  {
    *next_sel   = next_selectors;
    *n_next_sel = ARRAY_SIZE(next_selectors);
  }

  return rc;
}

int select_dirs(const char* path, const dir_selector_fn* dsfs, size_t ndsfs)
{
  DIR*  dp;
  int   rc  = -1;

  if (NULL != (dp = opendir(path)))
  {
    struct dirent*  de;
    char            next_path[PATH_MAX];

    rc = 0;

    do
    {
      size_t  s;

      errno = 0;  // 'cause readdir() is lame and readdir_r is deprecated
      de    = readdir(dp);
      if (NULL != de)
      {
        for (s = 0; s < ndsfs; s++)
        {
          const dir_selector_fn*  next_sel;
          size_t                  n_next_sel;

          if (dsfs[s](de->d_name, (const void**)&next_sel, &n_next_sel))
          {
            char* canon_name;
            snprintf(next_path, sizeof(next_path), "%s/%s", path, de->d_name);

            if (NULL != (canon_name = realpath(next_path, NULL)))
            {
              if (next_sel && n_next_sel)
                rc += select_dirs(canon_name, next_sel, n_next_sel);
              else
              {
                if (info_add_blk_dev(canon_name))
                  rc++;
              }
              free(canon_name);
            }
          }
        }
      }
      else if (0 != errno)
        perror("readdir");
    } while (NULL != de);

    if (rc == 0)
    {
      char* s = strrchr(path, '/');
      if (s)
      {
        if (scsi_host_dir_selector(s+1, NULL, NULL))
          info_add_host(path);
      }
    }
    closedir(dp);
  }

  return rc;
}

void
sysfs_init(
)
{
  long              name_max    = pathconf(SYSFS_PCI_BASE, _PC_NAME_MAX);
  size_t            de_base_len = offsetof(struct dirent, d_name);
  if (-1 == name_max)
    name_max = 255;

  de_size = de_base_len + name_max + 1;
}

int
sysfs_scan(
)
{
  dir_selector_fn   selectors[] = { pci_dev_dir_selector };

  return select_dirs(SYSFS_PCI_BASE, selectors, ARRAY_SIZE(selectors));
}

