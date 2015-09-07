ifneq ($(KERNELRELEASE),)
obj-m 	:= kl_usb_drv.o

else


ifdef BUILD_BBB
# BBB
KDIR ?= /opt/embedded/bbb/kernel/linux-dev-am33x-v3.18

else
# HOST
KDIR ?= /lib/modules/$(shell uname -r)/build

endif

PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

endif
