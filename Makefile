obj-m += mymodule14.o

#KERNEL_DIR = $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)
all:
	#$(MAKE) -C /lib/modules/$(KERNEL_DIR)/build M=$(PWD) modules
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *-
