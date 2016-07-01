KVERS = $(shell uname -r)
KDIR = /home/taxuewuhen/project/x4412/src/kernel
#  kernel modules
obj-m += btn_device.o
#  specify falgs for the module comilation
#EXTRA_CFLAGS=-g -O0


build: kernel_modules

kernel_modules:
	make -C $(KDIR) M=$(PWD) modules CROSS_COMPILE=arm-linux- ARCH=arm

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf *.c~
	chmod 777 -R ./


