ifneq ($(KERNELRELEASE),)
obj-m 	:= usb_test.o


else
# BBB
# KDIR ?= /opt/embedded/bbb/kernel/linux-dev-am33x-v3.18
# HOST
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

endif
