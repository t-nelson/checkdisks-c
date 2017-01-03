SUBDIRS = .

bin_PROGRAMS = checkdisks

checkdisks_SHORTNAME = .cd

MOSTLYCLEANFILES = .cd-*.o

checkdisks_CPPFLAGS = $(AM_CPPFLAGS) $(RLS_CPPFLAGS)
checkdisks_CFLAGS = $(RLS_CFLAGS) $(DBG_CFLAGS) $(DBUS_CFLAGS)
checkdisks_LDADD = $(DBUS_LIBS)

checkdisks_SOURCES= \
	blk_dev_info.c \
	info_manager.c \
	main.c \
	output_fmt.c \
	pci_slot.c \
	scsi_host.c \
	scsi_target.c \
	sysfs_scan.c \
	udisks2.c \
	utils.c
	
