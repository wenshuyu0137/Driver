CONFIG_MODULE_SIG=n
ifeq ($(KERNELRELEASE),)

ifeq ($(ARCH),arm)
	KERNELDIR ?= /home/wsy/fs-mp1a/stm32mp1-openstlinux-5.4-dunfell-mp1-20-06-24/sources/arm-ostl-linux-gnueabi/linux-stm32mp-5.4.31-r0/linux-5.4.31
	ROOTFS ?= /source/rootfs
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
endif
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) INSTALL_MOD_PATH=$(ROOTFS) modules_install
clean:
	rm -rf *.o *.ko .*.cmd *.mod.* modules.order Module.symvers .tmp_versions *.a
else

obj-m := mutex_led.o

endif
