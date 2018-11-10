obj-m += mymodule3.o

#KERNEL_DIR = $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)
all:
	#$(MAKE) -C /lib/modules/$(KERNEL_DIR)/build M=$(PWD) modules
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *-

#sudo rmmod mymodule3; sudo rm -r /dev/testDevice; make; sudo insmod mymodule3.ko; sudo mknod -m 666 /dev/testDevice c 498 0; gcc userapp2.c -o userapp; ./userapp 
