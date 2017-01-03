
#ifndef PCI_SLOT_H__
#define PCI_SLOT_H__

struct pci_slot;
struct scsi_host;

/* pci_addr should be on the heap */
struct pci_slot*
pci_slot_create(
          char*             pci_addr
);

void
pci_slot_destroy(
          struct pci_slot*  ps
);

void
pci_slot_add_sibling(
          struct pci_slot*  ps
  ,       struct pci_slot*  prev
  ,       struct pci_slot** head
);

struct pci_slot*
pci_slot_next(
    const struct pci_slot*  ps
);

void
pci_slot_add_host(
          struct pci_slot*  ps
  ,       struct scsi_host* sh
);

struct scsi_host*
pci_slot_get_first_host(
    const struct pci_slot*  ps
);

struct scsi_host*
pci_slot_find_host(
    const struct pci_slot*  ps
  ,       int               host_no
);

const char*
pci_slot_get_address(
    const struct pci_slot*  ps
);

#endif /* PCI_SLOT_H__ */

