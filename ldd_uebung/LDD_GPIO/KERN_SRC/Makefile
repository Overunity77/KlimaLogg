
ARCH=

ifdef TARGET
 export ARCH:=arm
 export CROSS_COMPILE:=arm-linux-
 KERN_SRC	:= /opt/embedded/bbb/rootfs/lib/modules/3.18.5+/build
else
 export ARCH:=x86_64
 export CROSS_COMPILE:=
 KERN_SRC	:= /lib/modules/$(shell uname -r)/build/
endif



obj-m		:= LDD_GPIO.o 

PWD			:= $(shell pwd)


modules:
	make -C $(KERN_SRC) M=$(PWD) modules


install:
	make -C $(KERN_SRC) M=$(PWD) modules_install
	depmod -a

clean:
	make -C $(KERN_SRC) M=$(PWD) clean
